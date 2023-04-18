#pragma once

#include "fu.h"
#include "log_fu.h"
#include "blob_fu.h"

#include "field_fu.h"
#include "db_fu.h"

#include "sys/wait.h"

#define MAIN_DB_FILE_APP "main.sqlite"

typedef struct {
    blob_t * dir;
    blob_t * state_dir;
    blob_t * main_db_file;
    db_t * db;
} app_fu_t;

app_fu_t app = {};

// get the directory of the AppImage AppDir for accessing resource files, etc
// in the AppImage
blob_t *
new_app_dir_fu()
{
    char exe[256];
    ssize_t len = readlink("/proc/self/exe", exe, 256);
    exe[len] = '\0';

    return const_blob(dirname(exe));
}

// close sqlite db, etc
int
pre_fork_app_fu()
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

    if (open_db(app.main_db_file, &app.db)) {
        exit(EXIT_FAILURE);
    }

    return 0;
}

pid_t
fork_app_fu()
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
set_default_sigaction_handlers()
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


/*
int
upgrade_db_app(const blob_t * db_file)
{
    // TODO(jason): originally, put the user table here, but then moved to web_fu.h
    // Not sure if this is necessary or what tables would be added here.
    version_sql_t versions[] = {
    };

    return upgrade_db(db_file, versions);
}
*/


void
exit_callback_app()
{
    flush_log();
}

int
init_app_fu(int argc, char *argv[], void (* flush_log_f)())
{
    atexit(exit_callback_app);

    if (argc < 2) {
        error_log("state directory required", "argc", 1);
        return -1;
    }

    app.dir = new_app_dir_fu();
    app.state_dir = const_blob(argv[1]);
    app.main_db_file = new_path_file_fu(app.state_dir, B(MAIN_DB_FILE_APP));

    if (dev_mode()) {
        debug("XXXXX running in DEV MODE XXXXX");
    }

    // NOTE(jason): according to the main page, this should be the default
    setbuf(stderr, NULL);

    if (set_default_sigaction_handlers()) {
        log_errno("failed to set default sigaction handlers");
        return -1;
    }

    init_log(flush_log_f);

    /*
    if (upgrade_db_app(app.main_db_file)) {
        return -1;
    }
    */

    flush_log();

    return 0;
}

