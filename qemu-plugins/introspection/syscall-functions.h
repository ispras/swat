#ifndef SYSCALL_FUNCTIONS_H
#define SYSCALL_FUNCTIONS_H

#define SYS_Unknown 0

enum SYS_Windows {
    SYS_NtClose = 1,
    SYS_NtCreateFile,
    SYS_NtCreateProcess,
    SYS_NtCreateProcessEx,
    SYS_NtCreateSection,
    SYS_NtOpenSection,
    SYS_NtMapViewOfSection,
    SYS_NtDuplicateObject,
    SYS_NtOpenFile,
    SYS_NtQueryInformationProcess,
    SYS_NtUnmapViewOfSection,
};

enum SYS_Linux {
    SYS_open = 1,
    SYS_close,
    SYS_creat,
    SYS_mmap,
    SYS_openat,
};

#endif // SYSCALLS_FUNCTIONS_H
