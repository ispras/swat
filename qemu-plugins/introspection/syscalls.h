#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stdbool.h>
#include "introspection.h"

typedef enum OSType {
	OS_UNKNOWN,
	OS_WINXP,
	OS_LINUX,
} OSType;

extern OSType os_type;

typedef struct SCData {
    uint32_t num;
    void *params;
} SCData;

typedef struct FileParams {
    address_t phandle;
    char *name;
} FileParams;

typedef struct DuplicateParams {
    File *f;
    address_t phandle;
    // TODO: process handle
} DuplicateParams;

typedef struct SectionParams {
    address_t phandle;
    char *name;
    File *file;
} SectionParams;

typedef struct MapSectionParams {
    Section *section;
    address_t pbase;
    address_t psize;
} MapSectionParams;

typedef struct MapFileParams {
    File *file;
    address_t base;
    address_t size;
    address_t offset;
} MapFileParams;

void sc_insert(context_t ctx, address_t sp, uint32_t num, void *param);
SCData *sc_find(context_t ctx, address_t sp);
void sc_erase(context_t ctx, address_t sp);

/* Functions for detecting and processing i386 syscalls */
bool is_syscall_i386(address_t pc, cpu_t cpu);
void syscall_i386(address_t pc, cpu_t cpu);

/* WinXP on i386 */
void *syscall_enter_winxp(uint32_t sc, address_t pc, cpu_t cpu);
void syscall_exit_winxp(SCData *sc, address_t pc, cpu_t cpu);

/* Linux on i386 (TODO: and others) */
void *syscall_enter_linux(uint32_t sc, address_t pc, cpu_t cpu);
void syscall_exit_linux(SCData *sc, address_t pc, cpu_t cpu);

#endif // SYSCALLS_H
