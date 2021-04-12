#ifndef LINUX_I386_SYSCALL_MAP_H
#define LINUX_I386_SYSCALL_MAP_H

static const uint32_t linux_i386_syscall_map[] = {
    [5] = SYS_open,
    [6] = SYS_close,
    [8] = SYS_creat,
    [11] = SYS_execve,
    [192] = SYS_mmap,
    [295] = SYS_openat,
};

#endif
