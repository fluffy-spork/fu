#pragma once

int
mkdir_file_fu(const blob_t * path, mode_t mode)
{
    if (mkdir(cstr_blob(path), mode)) {
        return -1;
    }

    return 0;
}

int
open_read_file_fu(const blob_t * path)
{
    return open(cstr_blob(path), O_RDONLY);
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
copy_file_fu(int out_fd, int in_fd, size_t len)
{
    // NOTE(jason): from https://stackoverflow.com/questions/7463689/most-efficient-way-to-copy-a-file-in-linux

    blob_t * buf = local_blob(8192);

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
write_prefix_file_fu(const blob_t * path, const blob_t * prefix, int in_fd, size_t count)
{
    log_var_blob(path);

    int out_fd = open(cstr_blob(path), O_CREAT|O_WRONLY|O_EXCL, S_IRUSR|S_IWUSR);
    if (out_fd == -1) {
        return log_errno("open");
    }

    ssize_t total = 0;
    ssize_t result = 0;

    result = write_file_fu(out_fd, prefix);
    if (result == -1) goto end;

    total += result;

    result = copy_file_fu(out_fd, in_fd, count - prefix->size);
    if (result == -1) goto end;

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

