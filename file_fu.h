#pragma once

#include <dlfcn.h> // dladdr, ..
#include <fcntl.h>
#ifdef __linux__
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
#include <sys/stat.h>


int
size_file_fu(int fd, size_t * size)
{
    struct stat st;
    if (fstat(fd, &st) == -1) {
        return log_errno("fstat");
    }

    *size = st.st_size;
    return 0;
}


int
add_path_file_fu(blob_t * path, const blob_t * name)
{
    assert_not_null(path);
    assert_not_null(name);

    if (!begins_u8_blob(name, '/') && !ends_u8_blob(path, '/')) {
        write_blob(path, "/", 1);
    }
    add_blob(path, name);

    return path->error;
}


int
path_file_fu(blob_t * path, const blob_t * dir, const blob_t * file)
{
    assert_not_null(path);
    assert_not_null(dir);
    assert_not_null(file);

    add_blob(path, dir);
    return add_path_file_fu(path, file);
}


int
id_path_file(blob_t * path, const blob_t * dir, const s64 id)
{
    assert_not_null(path);
    assert_not_null(dir);

    add_blob(path, dir);
    write_blob(path, "/", 1);
    add_s64_blob(path, id);
    return path->error;
}


blob_t *
new_path_file_fu(const blob_t * dir, const blob_t * file)
{
    blob_t * path = blob(256);
    if (path_file_fu(path, dir, file)) {
        free_blob(path);
        return NULL;
    }

    return path;
}

// what should happen if there's no '/'?  likely return empty string and not error
ssize_t
dirname_file_fu(const blob_t * path, blob_t * dirname)
{
    ssize_t index = rindex_blob(path, '/', path->size - 1);
    if (index == -1) {
        return -1;
    }

    return sub_blob(dirname, path, 0, index) >= 0 ? 0 : dirname->error;
}

// if the path separator doesn't exist returns the whole path
ssize_t
basename_file_fu(const blob_t * path, blob_t * basename)
{
    ssize_t index = rindex_blob(path, '/', -1);
    if (index == -1) {
        return add_blob(basename, path);
    }

    return sub_blob(basename, path, index + 1, -1) > 0 ? 0 : basename->error;
}


bool
exists_file(const blob_t * path)
{
    return access(cstr_blob(path), F_OK) == 0;
}


int
read_access_file_fu(const blob_t * path)
{
    return access(cstr_blob(path), R_OK);
}


int
write_access_file_fu(const blob_t * path)
{
    return access(cstr_blob(path), W_OK);
}


int
readwrite_access_file_fu(const blob_t * path)
{
    return access(cstr_blob(path), R_OK|W_OK);
}


int
execute_access_file_fu(const blob_t * path)
{
    return access(cstr_blob(path), X_OK);
}


int
mkdir_file_fu(const blob_t * path, mode_t mode)
{
    if (mkdir(cstr_blob(path), mode)) {
        return -1;
    }

    return 0;
}

// TODO(jason): could probably use a version without a mode as it seems to be
// S_IRWXU in current uses
int
ensure_dir_file_fu(const blob_t * path, mode_t mode)
{
    if (mkdir_file_fu(path, mode)) {
        if (errno != EEXIST) {
            return -1;
        }
    }

    return 0;
}

int
open_file_fu(const blob_t * path, int flags, int mode)
{
    int rc = open(cstr_blob(path), flags, mode);
    if (rc == -1) {
        log_errno(cstr_blob(path));
    }

    return rc;
}

int
open_read_file_fu(const blob_t * path)
{
    return open_file_fu(path, O_RDONLY, 0);
}

// TODO(jason): are the default permissions ok?
int
open_write_file_fu(const blob_t * path)
{
    return open_file_fu(path, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
}

int
open_trunc_file(const blob_t * path)
{
    return open_file_fu(path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
}

// TODO(jason): are the default permissions ok?
int
open_read_write_file_fu(const blob_t * path)
{
    return open_file_fu(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
}

int
close_file_fu(int fd)
{
    int rc = close(fd);
    if (rc == -1) {
        log_errno("close_file");
    }

    return rc;
}


error_t
delete_file(const blob_t * path)
{
    const char * cpath = cstr_blob(path);
    if (unlink(cpath)) {
        return log_errno(cpath);
    }

    return 0;
}


ssize_t
read_max_file_fu(int fd, blob_t * b, size_t max)
{
    size_t available;
    if (max == 0) {
        available = available_blob(b);
    }
    else {
        available = min_size(available_blob(b), max);
    }

    if (available == 0) {
        return 0;
    }

    u8 * buf = &b->data[b->size];

    // TODO(jason): it's possible there could be more to read that would fit
    // than is returned in one read call.  maybe use a do/while
    ssize_t size = read(fd, buf, available);
    if (size > 0) b->size += size;

    // size 0 means an EOF was read.
    return size;
}

ssize_t
read_file_fu(int fd, blob_t * b)
{
    return read_max_file_fu(fd, b, 0);
}

ssize_t
write_file_fu(int fd, const blob_t * b)
{
    return write(fd, b->data, b->size);
}

// writes until there's an error or full success.  retries on partial writes
ssize_t
write_full_file_fu(int fd, const blob_t * b)
{
    ssize_t ret = 0;

    size_t n = b->size;

    while (n) {
        ssize_t written = write(fd, b->data + ret, n);
        // TODO(jason): probably wrong.  needs to check for EINTR or other
        // non-fatal reasons for return with partial write
        if (written == -1) return log_errno("write full failed");

        n -= written;
        ret += written;
    }

    return ret;
}


ssize_t
copy_fd_file_fu(int out_fd, int in_fd, size_t len)
{
    // NOTE(jason): from https://stackoverflow.com/questions/7463689/most-efficient-way-to-copy-a-file-in-linux

    blob_t * buf = stk_blob(8192);

    ssize_t ret = 0;

    while (len) {
        reset_blob(buf);

        ssize_t read = read_file_fu(in_fd, buf);
        if (read == 0) break;
        if (read == -1) return log_errno("copy read");

        ssize_t written = write_full_file_fu(out_fd, buf);
        if (written == -1) return log_errno("copy write");

        len -= written;
        ret += written;
    }

    return ret;
}


ssize_t
copy_file_fu(const blob_t * out_path, const blob_t * in_path)
{
    int out_fd = open_read_write_file_fu(out_path);
    if (out_fd == -1) {
        return -1;
    }

    debug_blob(in_path);
    int in_fd = open_read_file_fu(in_path);
    if (in_fd == -1) {
        return -1;
    }

    size_t len = 0;
    if (size_file_fu(in_fd, &len)) {
        return -1;
    }

    ssize_t rc = copy_fd_file_fu(out_fd, in_fd, len);
    if (rc == -1) {
        log_errno("copy_fd_file_fu");
    }

    return rc;
}


int
set_tcp_cork_file(int fd, int cork)
{
#ifdef __linux__
    return setsockopt(fd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));
#else
    UNUSED(fd);
    UNUSED(cork);
    return 0;
#endif
}

// TODO(jason): possibly better to make this interface more like Apple sendfile
// with header and trailer iovec.  Could wrap with TCP_CORK on linux too.
ssize_t
send_file(int out_fd, int in_fd, size_t len)
{
#ifdef __linux__
    return sendfile(out_fd, in_fd, NULL, len);
#else
    return copy_fd_file_fu(out_fd, in_fd, len);
#endif
}

/* none of this is tested and may be unused
int
tmp_file_fu()
{
    // NOTE(jason): from Linux Programming Inteface book
    int fd;
    char template[] = "/tmp/somestringXXXXXX";
    fd = mkstemp(template);
    if (fd == -1) {
        return log_errno("mkstemp");
    }

    debugf("generated filename was: %s\n", template);
    // Name disappears immediately, but the file
    // is removed only after close()
    unlink(template);

    return fd;
}
*/

// count should be prefix->size + <whatever from fd>
ssize_t
write_prefix_file_fu(const blob_t * path, const blob_t * prefix, const int in_fd, const size_t count)
{
    //debug_blob(path);

    const int out_fd = open(cstr_blob(path), O_CREAT|O_WRONLY|O_EXCL, S_IRUSR|S_IWUSR);
    if (out_fd == -1) {
        return log_errno("open");
    }

    ssize_t total = 0;
    ssize_t result = 0;

    result = write_file_fu(out_fd, prefix);
    if (result == -1) goto end;

    total += result;

    if ((size_t)total == count) goto end;

    ssize_t unread = count - prefix->size;
    if (unread > 0) {
        result = copy_fd_file_fu(out_fd, in_fd, unread);
        if (result == -1) goto end;
    }

    total += result;

    result = fsync(out_fd);

end:
    if (close(out_fd) == -1) {
        log_errno("close");
    }

    if (result == -1) {
        return log_errno("fsync");
    }

    assert((size_t)total == count);

    return total;
}

// NOTE(jason): overwrites data from beginning
int
load_file(const blob_t * path, blob_t * data)
{
    int fd = open_read_file_fu(path);
    if (fd == -1) {
        return log_errno("open_read_file_fu");
    }

    size_t size = 0;
    if (size_file_fu(fd, &size)) {
        return log_errno("size_file_fu");
    }

    reset_blob(data);

    if (size > available_blob(data)) {
        errno = ENOSPC;
        log_errno("file is too large");
        return -1;
    }

    if (read_file_fu(fd, data) == -1) {
        return -1;
    }

    return 0;
}

// NOTE(jason): overwrites data from beginning
int
save_file(const blob_t * path, const blob_t * data)
{
	int fd = open_trunc_file(path);
	if (fd == -1) {
		return log_errno("open_write_file_fu");
	}

	if (write_full_file_fu(fd, data) == -1) {
        return -1;
	}

    close(fd);

	return 0;
}


// current working directory
int
cwd_file_fu(blob_t * path)
{
    if (!getcwd((void *)path->data, path->capacity)) {
        return -1;
    }

    path->size = strlen((void *)path->data);

    return 0;
}


int
executable_path_file_fu(blob_t * path)
{
    char exe[256];
    ssize_t len = readlink("/proc/self/exe", exe, 256);
    if (len == -1) {
        // NOTE(jason): https://stackoverflow.com/questions/799679/programmatically-retrieving-the-absolute-path-of-an-os-x-command-line-app
        // not linux
        // only for macos command line apps, not app bundles
        void * pmain = dlsym(RTLD_DEFAULT, "main");
        if (!pmain) {
            debugf("dlsym: %s", dlerror());
            return -1;
        }

        Dl_info info;
        if (dladdr(pmain, &info) == 0) {
            debugf("error: dladdr: %s", dlerror());
            return -1;
        }

        add_cstr_blob(path, info.dli_fname);
    }
    else {
        write_blob(path, exe, len);
    }

//    debug_blob(path);

    return 0;
}

