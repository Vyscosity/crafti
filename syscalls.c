/*
 * Minimal syscalls stubs for bare-metal ARM newlib cross-compilation
 * Required for linking with arm-none-eabi toolchain
 */

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* Implement _kill - send signal to process */
int _kill(int pid, int sig) {
    errno = ENOSYS;
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
extern char _end;  /* Symbol provided by linker script */

static char *heap_end = &_end;

void *_sbrk(int incr) {
    char *prev_heap_end;

    prev_heap_end = heap_end;
    heap_end += incr;

    return (void *)prev_heap_end;
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
