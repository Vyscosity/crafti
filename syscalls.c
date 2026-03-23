/*
 * Minimal syscalls stubs for bare-metal ARM newlib cross-compilation
 * Required for linking with arm-none-eabi toolchain on Nspire TI calculator
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

/* Implement _kill - send signal to process */
int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

/* Implement _getpid - get process ID */
int _getpid(void) {
    return 1;
}

/* Implement _exit - exit process */
void _exit(int status) {
    while (1) {}
}

/* Implement _write - write to file */
int _write(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    /* Writing is a no-op on bare-metal; could stub to UART on real hardware */
    return len;
}

/* Implement _close - close file */
int _close(int file) {
    (void)file;
    return -1;
}

/* Implement _fstat - get file status */
int _fstat(int file, struct stat *st) {
    (void)file;
    if (st) {
        st->st_mode = S_IFCHR;
    }
    return 0;
}

/* Implement _isatty - check if file is terminal */
int _isatty(int file) {
    (void)file;
    return 1;  /* Assume we're always a "terminal" on bare-metal */
}

/* Implement _lseek - seek in file */
long _lseek(int file, long ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

/* Implement _sbrk - allocate memory from heap */
void *_sbrk(int incr) {
    /* On bare-metal targets without a linker script providing _end,
     * just return an error. Most embedded systems don't use malloc anyway. */
    (void)incr;
    errno = ENOMEM;
    return (void *)-1;
}

/* Implement _read - read from file */
int _read(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    return 0;  /* No input available on bare-metal */
}

/* Implement _open - open file */
int _open(const char *name, int flags, int mode) {
    (void)name;
    (void)flags;
    (void)mode;
    return -1;
}

/* Implement _link - create hard link */
int _link(const char *old, const char *new) {
    (void)old;
    (void)new;
    errno = EMLINK;
    return -1;
}

/* Implement _unlink - remove file */
int _unlink(const char *name) {
    (void)name;
    errno = ENOENT;
    return -1;
}

/* Implement _stat - get file info */
int _stat(const char *file, struct stat *st) {
    (void)file;
    if (st) {
        st->st_mode = S_IFCHR;
    }
    return 0;
}

/* Implement _rename - rename file */
int _rename(const char *oldname, const char *newname) {
    (void)oldname;
    (void)newname;
    errno = ENOSYS;
    return -1;
}

/* Implement _mkdir - create directory */
int _mkdir(const char *path, mode_t mode) {
    (void)path;
    (void)mode;
    errno = ENOSYS;
    return -1;
}

/* Implement _chdir - change directory */
int _chdir(const char *path) {
    (void)path;
    errno = ENOSYS;
    return -1;
}
