/*
 * Minimal syscalls stubs for bare-metal ARM newlib cross-compilation
 * Required for linking with arm-none-eabi toolchain on Nspire TI calculator
 * 
 * These are minimal implementations to satisfy linker requirements.
 * No standard library headers are included to ensure compatibility with
 * bare-metal cross-compilation environments.
 */

/* External declarations without relying on system headers */
typedef int pid_t;
typedef long off_t;
typedef unsigned int mode_t;
typedef int ptrdiff_t;

struct stat {
    unsigned long st_dev;
    unsigned long st_ino;
    unsigned int st_mode;
    unsigned int st_nlink;
    unsigned int st_uid;
    unsigned int st_gid;
    unsigned long st_rdev;
    long st_size;
    long st_blksize;
    long st_blocks;
    long st_atime;
    long st_mtime;
    long st_ctime;
};

#define S_IFCHR 0x2000

/* Implement _exit - exit process */
void _exit(int status) {
    (void)status;
    while (1) {}
}

/* Implement _write - write to file */
int _write(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    /* Writing is a no-op on bare-metal; could stub to UART on real hardware */
    return len;
}

/* Implement _sbrk - allocate memory from heap */
void *_sbrk(int incr) {
    (void)incr;
    return (void *)-1;
}

/* Implement _getpid - get process ID */
int _getpid(void) {
    return 1;
}

/* Implement _kill - send signal to process */
int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    return -1;
}

/* Implement _lseek - seek in file */
off_t _lseek(int file, off_t ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
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

/* Implement _close - close file */
int _close(int file) {
    (void)file;
    return -1;
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
    return -1;
}

/* Implement _unlink - remove file */
int _unlink(const char *name) {
    (void)name;
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
    return -1;
}

/* Implement _mkdir - create directory */
int _mkdir(const char *path, mode_t mode) {
    (void)path;
    (void)mode;
    return -1;
}

/* Implement _chdir - change directory */
int _chdir(const char *path) {
    (void)path;
    return -1;
}
