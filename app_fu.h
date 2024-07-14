#pragma once

#include "fu.h"
#include "log_fu.h"
#include "blob_fu.h"

#include "field_fu.h"
#include "db_fu.h"

#include "sys/wait.h"

#define MAIN_DB_FILE_APP "main.sqlite"

typedef struct {
    blob_t * name;

    blob_t * dir;
    blob_t * state_dir;

    blob_t * main_db_file;
    db_t * db;
} app_fu_t;

app_fu_t app = {};

#define RES_APP(var, E) \
    E(user_table, "user", var) \
    E(sql_dir, "/sql", var) \

ENUM_BLOB(res_app, RES_APP)

// get the directory of the AppImage AppDir for accessing resource files, etc
// in the AppImage
blob_t *
new_app_dir_fu(void)
{
    char exe[256];
    ssize_t len = readlink("/proc/self/exe", exe, 256);
    if (len == -1) return NULL;

    exe[len] = '\0';

    blob_t * dir = const_blob(dirname(exe));
    blob_t * app_dir = new_path_file_fu(dir, B("AppDir"));

    if (read_access_file_fu(app_dir)) {
        // app image
        free_blob(app_dir);
        return dir;
    }
    else {
        // file system
        dir->constant = false;
        free_blob(dir);
        return app_dir;
    }
}

blob_t *
new_state_path_app(const blob_t * subpath)
{
    return new_path_file_fu(app.state_dir, subpath);
}

int
state_path_app(blob_t * path, const blob_t * subpath)
{
    return path_file_fu(path, app.state_dir, subpath);
}

int
path_dir_app(blob_t * path, const blob_t * subpath)
{
    return path_file_fu(path, app.dir, subpath);
}

// NOTE(jason): overwrites data from beginning
int
load_file_app(const blob_t * subpath, blob_t * data)
{
    blob_t * path = stk_blob(256);
    path_dir_app(path, subpath);
    return load_file(path, data);
}

int
load_sql_file_app(const blob_t * file, blob_t * data)
{
    blob_t * path = stk_blob(256);
    add_blob(path, res_app.sql_dir);
    add_path_file_fu(path, file);
    return load_file_app(path, data);
}

#define require_sql_app(file, stmt) dev_error(load_sql_app(file, stmt))

int
load_sql_app(const blob_t * file, stmt_db_t ** stmt)
{
    blob_t * sql = stk_blob(4096);
    if (load_sql_file_app(file, sql)) {
        return -1;
    }

    if (prepare_db(app.db, stmt, sql)) {
        return -1;
    }

    return 0;
}

int
open_main_db_app(void)
{
    return open_db(app.main_db_file, &app.db);
}

int
save_state_file_app(const blob_t * subpath, const blob_t * data)
{
    blob_t * path = stk_blob(256);
    state_path_app(path, subpath);
    return save_file(path, data);
}

// close sqlite db, etc
int
pre_fork_app_fu(void)
{
    flush_log();

    if (app.db) {
        error_log("main db is open in parent", "fork", 2);
    }

    return 0;
}

// called after fork() for parent and child
// open sqlite, etc
int
post_fork_app_fu(pid_t pid)
{
    UNUSED(pid);

    //open main db
    if (app.db) {
        error_log("main db already open in child", "fork", 3);
        exit(EXIT_FAILURE);
    }

    if (open_main_db_app()) {
        exit(EXIT_FAILURE);
    }

    return 0;
}

pid_t
fork_app_fu(void)
{
    if (pre_fork_app_fu()) {
        log_errno("pre_fork_app");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        log_errno("fork");
        return -1;
    }

    // parent doesn't get the db opened, etc
    if (pid != 0) return pid;

    if (post_fork_app_fu(pid)) {
        log_errno("post_fork_app");
        return -1;
    }

    return pid;
}

void
sigchld_sa_handler(int signum)
{
    // TODO(jason): unused and unclear if it's currently necessary since
    // web_main loops checking for children
    int saved_errno = errno;

    UNUSED(signum);

    info_log("sigchild (could be from popen)");

    // seems like we don't need this handler and the parent main could just
    // loop on wait and reap children without signals
    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

/*
volatile sig_atomic_t fatal_error_in_progress = 0;

void
fatal_signal_handler(int signum)
{
    // https://www.gnu.org/software/libc/manual/html_node/Termination-in-Handler.html

    if (fatal_error_in_progress) raise(signum);

    fatal_error_in_progress = 1;

    int saved_errno = errno;

    debug_backtrace_fu();

    info_log(strsignal(signum));
    //error_log("SIGABRT", "signal", saved_errno);

    flush_log();

    killpg(0, signum);

    errno = saved_errno;

    // causes the default behavior for exiting the process to happen
    // TODO(jason): possibly not correct since using sigaction?
    signal(signum, SIG_DFL);
    raise(signum);
}
*/

// mainly to dump the log before quitting
void
sigabrt_sa_handler(int signum)
{
    UNUSED(signum);

    int saved_errno = errno;

    debug_backtrace_fu();

    //info_log(strsignal(signum));
    error_log("SIGABRT", "signal", saved_errno);

    flush_log();

    errno = saved_errno;
}

// TODO(jason): something wonky with this when working with systemd.  there are
// a bunch of messages about core dumps from worker children I think.  I don't
// know if it matters, but don't have time now.
//
// TODO(jason): evaluate closing main db on exit
// TODO(jason): can all signal handlers be combined into one when they're
// exiting
//
// this handler is currently still set on the worker children so they will also
// attempt to kill any children.  that could be something like ffmpeg
// transcoding a video, etc.
void
sigterm_sa_handler(int signum)
{
    UNUSED(signum);

    flush_log();

    killpg(0, SIGTERM);

    _exit(EXIT_SUCCESS);
}

void
sigsegv_sa_handler(int signum)
{
    info_log(strsignal(signum));

    debug_backtrace_fu();

    flush_log();

    _exit(EXIT_FAILURE);
}

int
set_default_sigaction_handlers(void)
{
    // TODO(jason): this is probably wrong, but mostly shouldn't be called

    struct sigaction sa;
    stack_t sigstack;

    sigstack.ss_sp = malloc(SIGSTKSZ);
    if (sigstack.ss_sp == NULL) {
        log_errno("malloc");
        exit(EXIT_FAILURE);
    }

    sigstack.ss_size = SIGSTKSZ;
    sigstack.ss_flags = 0;
    if (sigaltstack(&sigstack, NULL)) {
        log_errno("sigaltstack");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigsegv_sa_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_ONSTACK;
    if (sigaction(SIGSEGV, &sa, NULL)) {
        log_errno("sigaction");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigabrt_sa_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGABRT, &sa, NULL)) {
        log_errno("sigaction");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigterm_sa_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa, NULL)) {
        log_errno("sigaction");
        exit(EXIT_FAILURE);
    }

    return 0;
}


int
upgrade_db_app(const blob_t * db_file)
{
    // TODO(jason): does the user table really need to be user_app?
    // I feel like it's ok since it's at the base app level
    version_sql_t versions[] = {
        { 1, "create table if not exists user (user_id integer primary key autoincrement, email text unique not null, alias text unique not null, created integer not null default (unixepoch()), modified integer not null default (unixepoch())) strict" }
        , { 2, "alter table user add column invited_by_user_id integer not null default 0" }
    };

    return upgrade_db(db_file, versions);
}

int
insert_user_app(db_t * db, s64 * user_id, blob_t * email, blob_t * alias, s64 invited_by_user_id)
{
    return exec_s64_pbbi_db(db, B("insert into user (email, alias, invited_by_user_id, created, modified, last_read_post_id) values (?, ?, ?, unixepoch(), unixepoch(), (select coalesce(max(post_id),0) from post)) returning user_id"), user_id, email, alias, invited_by_user_id);
}

void
exit_callback_app(void)
{
    flush_log();
}

int
init_app_fu(const char * state_dir, void (* flush_log_f)(void))
{
    atexit(exit_callback_app);

    app.name = blob(32);

    app.dir = new_app_dir_fu();
    app.state_dir = const_blob(state_dir);
    app.main_db_file = new_path_file_fu(app.state_dir, B(MAIN_DB_FILE_APP));

    // NOTE(jason): has to be before any log calls
    init_log(flush_log_f);

    if (read_access_file_fu(app.state_dir)) {
        log_var_blob(app.state_dir);
        log_errno("cannot access state dir");
        return -1;
    }

    if (dev_mode()) {
        debug("XXXXX running in DEV MODE XXXXX");
    }

    // NOTE(jason): according to the man page, this should be the default
    setbuf(stderr, NULL);

    if (set_default_sigaction_handlers()) {
        log_errno("failed to set default sigaction handlers");
        return -1;
    }

    init_res_app();

    init_db();

    if (upgrade_db_app(app.main_db_file)) {
        error_log("db upgrade failed", "app", 1);
        return -1;
    }

    flush_log();

    return 0;
}

int
init_argv_app_fu(int argc, char *argv[], void (* flush_log_f)(void))
{
    if (argc < 2) {
        error_log("state directory required", "argc", 1);
        return -1;
    }

    return init_app_fu(argv[1], flush_log_f);
}

