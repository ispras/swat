#ifndef SYSCALLS_WINDOWS_H
#define SYSCALLS_WINDOWS_H

enum {
    SYS_Unknown = 0,
    SYS_NtClose,
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

#endif // SYSCALLS_WINDOWS_H
