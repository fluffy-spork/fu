#pragma once

#include "fu.h"
#include "log_fu.h"
#include "blob_fu.h"

#include "field_fu.h"
#include "db_fu.h"

#include "http.h"
#include "s3.h"

#include <sys/wait.h>
#include <sys/utsname.h>

#define MAIN_SQLITE_APP "main.sqlite"
#define BACKUPS_DIR_APP "backups"
#define BACKUPS_MAIN_SQLITE_APP BACKUPS_DIR_APP"/"MAIN_SQLITE_APP

typedef struct {
    blob_t * name;

    blob_t * dir;
    blob_t * state_dir;

    blob_t * main_db_file;
    db_t * db;

    s3_t * s3;
} app_fu_t;

app_fu_t app = {};

#define RES_APP(var, E) \
    E(user_table, "user", var) \
    E(sql_dir, "/sql", var) \


ENUM_BLOB(res_app, RES_APP)


// uname -s -m (for example, Linux-x86_64, Darwin-x86_64)
//
// NOTE(jason): initially for deploying multiple ffmpeg binaries with a
// platform suffix
void
add_platform_suffix_app(blob_t * b)
{
    // TODO(jason): could probably set this at init or otherwise cache, but it
    // shouldn't really be used frequently

    struct utsname name;
    if (uname(&name)) {
        log_errno("uname");
        dev_error("uname failed");
    }

    write_blob(b, "-", 1);
    add_blob(b, B(name.sysname));
    write_blob(b, "-", 1);
    // always use the x86_64 for arm since no prebuilt ffmpeg binaries for arm
    // and x86_64 works fine
    add_blob(b, B("x86_64"));
//    add_blob(b, B(name.machine));
}


// get the directory of the AppImage AppDir for accessing resource files, etc
// in the AppImage
int
app_dir_app(blob_t * path)
{
    blob_t * dir = stk_blob(256);

    char * appdir_env = getenv("APPDIR");
    if (appdir_env) {
        add_cstr_blob(dir, appdir_env);

        if (read_access_file_fu(dir)) {
            log_errno(cstr_blob(dir));
            return -1;
        }
        else {
            add_blob(path, dir);
        }

        return 0;
    }

    blob_t * app_dir = stk_blob(256);
    //blob_t * exe_path = stk_blob(256);

    if (cwd_file_fu(dir)) {
        return -1;
    }

    debug_blob(dir);

    /*
    if (executable_path_file_fu(exe_path)) {
    }
    else if (dirname_file_fu(exe_path, dir)) {
        return -1;
    }
    */

    if (path_file_fu(app_dir, dir, B("AppDir"))) {
        return -1;
    }

    if (read_access_file_fu(app_dir)) {
        // appimage assumed because AppDir doesn't exist
//        log_errno(cstr_blob(app_dir));
        add_blob(path, dir);
    }
    else {
        add_blob(path, app_dir);
    }

    return 0;
}

// TODO(jason): "state" should probably be "data" because that's the default directory name
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
backup_main_db_app(void)
{
    // TODO(jason):  gzip backup

    // TODO(jason): on-write or periodic backups
    // periodic backup with incremental diff backups on writes?
    // restore more complicated but just base.sqlite and main.sqlite.diff or something
    // So with infrequent writes the diff should be small

    db_t * db;
    if (open_db(app.main_db_file, &db)) {
        return log_errno("open db for backup");
    }

    int rc = 0;

    blob_t * backup_dir = stk_blob(1024);
    state_path_app(backup_dir, B(BACKUPS_DIR_APP));

    blob_t * backup_file = stk_blob(1024);
    state_path_app(backup_file, B(BACKUPS_MAIN_SQLITE_APP));

    if (ensure_dir_file_fu(backup_dir, S_IRWXU)) {
        rc = log_errno("unable to make backup dir");
        goto cleanup;
    }

    // NOTE(jason): vacuum into won't work if a file already exists
    // If the file exists a backup is already in progress or crashed.  this might prevent multiple 
    if (exists_file(backup_file)) {
//        delete_file(backup_file);
        rc = log_errno("skipping backup, file exists");
        goto cleanup;
    }

    info_log("backup main db begin");

    // vacuum into backup file
    rc = vacuum_into_file_db(db, backup_file);
    if (rc) {
        log_error_db(db, "db backup failed");
        goto cleanup;
    }

    rc = put_file_s3(B(BACKUPS_MAIN_SQLITE_APP), backup_file, B("application/vnd.sqlite3"), no_store_cache_control_http, app.s3);

cleanup:
    delete_file(backup_file);

    close_db(db);

    info_log("backup main db end");

    return rc;
}


int
restore_main_db_app (s3_t * s3)
{
    if (exists_file(app.main_db_file)) {
        info_log("using existing main.sqlite");
        return 0;
    }

    // TODO(jason): print progress to log? mostly small dbs so is almost
    // instantaneous anyway

    long status_code = 0;
    if (get_file_status_code_s3(B(BACKUPS_MAIN_SQLITE_APP), app.main_db_file, &status_code, s3)) {
        if (status_code == 404) {
            info_log("no backup available");
        }
        else {
            return log_errno("failed to download backup database");
        }
    }
    else {
        info_log("main.sqlite downloaded from backup");
    }

    return 0;
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

// called after fork() in child
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
    // NOTE(jason): this is only called when exit is called or main returns,
    // not signals.  probably shouldn't use.
    atexit(exit_callback_app);

    // NOTE(jason): has to be before any log calls
    init_log(flush_log_f);

    int rc = 0;

    app.name = blob(32);

    app.dir = blob(256);
    if (app_dir_app(app.dir)) {
        error_log("unable to access app.dir", "app", 2);
        rc = -1;
        goto cleanup;
    }

    app.state_dir = const_blob(state_dir);
    app.main_db_file = new_path_file_fu(app.state_dir, B(MAIN_SQLITE_APP));

    if (readwrite_access_file_fu(app.state_dir)) {
        log_var_blob(app.state_dir);
        rc = log_errno("cannot access state dir");
        goto cleanup;
    }

    if (dev_mode()) {
        debug("XXXXX running in DEV MODE XXXXX");
    }

    // NOTE(jason): according to the man page, this should be the default
    setbuf(stderr, NULL);

    if (set_default_sigaction_handlers()) {
        rc = log_errno("failed to set default sigaction handlers");
        goto cleanup;
    }

    init_res_app();

    init_db();

    init_http();

    app.s3 = new_default_s3();

    if (restore_main_db_app(app.s3)) {
        rc = log_errno("db restore failed");
        goto cleanup;
    }

    if (upgrade_db_app(app.main_db_file)) {
        error_log("db upgrade failed", "app", 1);
        rc = -1;
        goto cleanup;
    }

cleanup:
    flush_log();

    return rc;
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

