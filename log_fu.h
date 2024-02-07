#pragma once

// batch logs.
// no allocs on adding log msg.
// delay formatting until written
// explicitly write logs to output, for example all logs for a request after the response is sent

#define error_log(msg, label, error) \
    _log(plog, msg, strlen(msg), label, strlen(label), error, __FILE__, __func__, __LINE__);

#define info_log(msg) \
    _log(plog, msg, strlen(msg), NULL, 0, 0, __FILE__, __func__, __LINE__);

#define log_var_cstr(cstr) \
    _log(plog, cstr, strlen(cstr), #cstr, strlen(#cstr), 0, __FILE__, __func__, __LINE__);

#define log_errno(msg) \
    _log_errno(plog, msg, strlen(msg), __FILE__, __func__, __LINE__);

#define log_s64(value, label) \
    _log_s64(plog, value, label, strlen(label), __FILE__, __func__, __LINE__);

#define log_u64(value, label) \
    _log_u64(plog, value, label, strlen(label), __FILE__, __func__, __LINE__);

#define log_var_u64(var) log_u64(var, #var)
#define log_var_s64(var) log_s64(var, #var)

#define timestamp_log(ts) \
    _timestamp_log(plog, ts);

// log an elapsed time since the provided start time and now
#define elapsed_log(start, label) \
    _elapsed_log(plog, start, label, strlen(label), __FILE__, __func__, __LINE__);

#define MAX_ENTRIES_LOG 64
#define MAX_MSG_LOG 1024
#define MAX_LABEL_LOG 64

// file and line number?
// should there be an integer, float/double, or number value stored for logging values
// that could be searched?
typedef struct {
    struct timespec timestamp;
    s32 error;
    // short descriptor like a variable name, etc. keeps it separate from the
    // value which may make it more searchable and avoids having to format the
    // msg when adding an entry.
    char label[MAX_LABEL_LOG];
    char msg[MAX_MSG_LOG];

    char * file;
    char * function;
    s64 line;
} log_entry_t;

typedef struct {
    // what size should this be?  doesn't really matter I guess
    size_t size;
    log_entry_t entries[MAX_ENTRIES_LOG];
} log_t;

// NOTE(jason): this is just for the initialization
//log_t _the_one_true_log = {};
// log is a C standard function for logarithm
log_t * plog = &(log_t){};

void (* _flush_log_f)(void);

int
init_log(void (* flush_log_f)(void))
{
    _flush_log_f = flush_log_f;

    return 0;
}

// TODO(jason): maybe make this for default plog?
void
erase_log(log_t * log)
{
    memset(log, 0, sizeof(log_t));
}

// get a timestamp using the same clock as the log.  Useful when logging
// elapsed times
void
_timestamp_log(log_t * log, struct timespec * ts)
{
    (void)log; // ignore for now, but possibly store clock_id in log_t
    clock_gettime(CLOCK_REALTIME, ts);
}

// TODO(jason): do I need a size for label?  why didn't I do that in the first
// place
// file and function aren't copied as it's assumed they're using the macros
void
_log(log_t * log, const void * msg, size_t size_msg, const char * label, size_t size_label, s32 error, const char * file, const char * function, s64 line)
{
    if (log->size < MAX_ENTRIES_LOG) {
        log_entry_t * entry = &log->entries[log->size];

        _timestamp_log(log, &entry->timestamp);

        copy_cstr(entry->msg, MAX_MSG_LOG, msg, size_msg);

        if (label) {
            copy_cstr(entry->label, MAX_LABEL_LOG, label, size_label);
        }

        entry->error = error;

        entry->file = (char *)file;
        entry->function = (char *)function;
        entry->line = line;
    }

    log->size++;
}

// Usage: return log_errno("open");
int
_log_errno(log_t * log, const char * msg, size_t size_msg, const char * file, const char * function, s64 line)
{
    char * s = strerror(errno);
    // NOTE(jason): I think I would prefer this, but it's a GNU extension and
    // isn't working for me for some reason.
    //char * s = strerrorname_np(errno);

    _log(log, msg, size_msg, s, strlen(s), errno, file, function, line);

    return -1;
}

void
_log_s64(log_t * log, const s64 value, const char * label, size_t size_label, const char * file, const char * function, s64 line)
{
    char s[256];
    // XXX: seems kind of sucky to use snprintf for number conversion
    int size = snprintf(s, 256, "%" PRId64, value);
    _log(log, s, size, label, size_label, 0, file, function, line);
}

void
_log_u64(log_t * log, const u64 value, const char * label, size_t size_label, const char * file, const char * function, s64 line)
{
    char s[256];
    // XXX: seems kind of sucky to use snprintf for number conversion
    int size = snprintf(s, 256, "%" PRIu64, value);
    _log(log, s, size, label, size_label, 0, file, function, line);
}

void
_elapsed_log(log_t * log, const struct timespec start, const char * label, size_t size_label, const char * file, const char * function, s64 line)
{
    struct timespec now;
    _timestamp_log(log, &now);

    struct timespec elapsed;

    sub_timespec(&now, &start, &elapsed);

    char msg[2048];
    size_t size = snprintf(msg, 2048, "elapsed %lld.%09ld", (long long)elapsed.tv_sec, elapsed.tv_nsec);

    _log(log, msg, size, label, size_label, 0, file, function, line);
}

// TODO(jason): I'm a bit skeptical about the resolution of the timestamps and
// if it's useful/necessary to theoretically have nanosecond precision
void
write_log(log_t * log, int fd)
{
    const int max_msg = 2048;

    char msg[max_msg];

    size_t imax = min_size(log->size, MAX_ENTRIES_LOG);

    // TODO(jason): change this to write everything into a single buffer and write once
    for (size_t i = 0; i < imax; i++) {
        log_entry_t * entry = &log->entries[i];
        size_t size;
        if (entry->label[0]) {
            size = snprintf(msg, max_msg, "%.4d %lld.%.9ld %s:%s:%zd %s: %s\n",
                    entry->error, (long long)entry->timestamp.tv_sec, entry->timestamp.tv_nsec, entry->file, entry->function, entry->line, entry->label, entry->msg);
        } else {
            size = snprintf(msg, max_msg, "%.4d %lld.%.9ld %s:%s:%zd %s\n",
                    entry->error, (long long)entry->timestamp.tv_sec, entry->timestamp.tv_nsec, entry->file, entry->function, entry->line, entry->msg);
        }
        ssize_t written = write(fd, msg, size);
        assert(written != -1);
    }
}

void
stderr_flush_log(void)
{
    if (plog->size < 1) return;

    write_log(plog, 2); // stderr
    erase_log(plog);
}

void
flush_log(void)
{
    if (_flush_log_f) {
        _flush_log_f();
        return;
    }

    stderr_flush_log();
}

