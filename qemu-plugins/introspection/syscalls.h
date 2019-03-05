#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stdbool.h>
#include "introspection.h"

typedef struct SCData {
    uint32_t num;
    void *params;
} SCData;

void sc_insert(context_t ctx, address_t sp, uint32_t num, void *param);
SCData *sc_find(context_t ctx, address_t sp);
void sc_erase(context_t ctx, address_t sp);

/* Functions for detecting and processing i386 syscalls */
bool is_syscall_i386(address_t pc, cpu_t cpu);
bool syscall_i386(address_t pc, cpu_t cpu);

#endif // SYSCALLS_H
