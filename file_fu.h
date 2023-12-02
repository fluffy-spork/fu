#pragma once

#include <fcntl.h>
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

    write_blob(path, "/", 1);
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

// what should happen if there's no '/'
int
dirname_file_fu(const blob_t * path, blob_t * dirname)
{
    ssize_t index = rindex_blob(path, '/', path->size - 1);
    if (index == -1) {
        return -1;
    }

    return sub_blob(dirname, path, 0, index) >= 0 ? 0 : dirname->error;
}

// if the path separator doesn't exist returns the whole path
int
basename_file_fu(const blob_t * path, blob_t * basename)
{
    ssize_t index = rindex_blob(path, '/', -1);
    if (index == -1) {
        return add_blob(basename, path);
    }

    return sub_blob(basename, path, index + 1, -1) > 0 ? 0 : basename->error;
}

int
read_access_file_fu(const blob_t * path)
{
    return access(cstr_blob(path), R_OK);
}

int
mkdir_file_fu(const blob_t * path, mode_t mode)
{
    if (mkdir(cstr_blob(path), mode)) {
        return -1;
    }

    return 0;
}

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

// TODO(jason): are the default permissions ok?
int
open_read_write_file_fu(const blob_t * path)
{
    return open_file_fu(path, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
}

ssize_t
read_file_fu(int fd, blob_t * b)
{
    size_t available = available_blob(b);
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
        ssize_t written = write_file_fu(fd, b);
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
copy_file_fu(const blob_t * out, const blob_t * in)
{
    int out_fd = open_read_write_file_fu(out);
    if (out_fd == -1) {
        return -1;
    }

    debug_blob(in);
    int in_fd = open_read_file_fu(in);
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
        error_log("file is too large", "db", size);
        errno = ENOSPC;
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
	int fd = open_write_file_fu(path);
	if (fd == -1) {
		return log_errno("open_write_file_fu");
	}

	if (write_full_file_fu(fd, data) == -1) {
        return -1;
	}

	return 0;
}

