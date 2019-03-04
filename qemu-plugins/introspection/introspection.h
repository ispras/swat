#ifndef INTROSPECTION_H
#define INTROSPECTION_H

#include <stdint.h>
#include <stdbool.h>

typedef void * cpu_t;
typedef uint64_t address_t;

/* Execution context */
typedef uint64_t context_t;
context_t vmi_get_context(cpu_t cpu);
address_t vmi_get_stack_pointer(cpu_t cpu);

/* System calls */
void syscall_init(void);
bool syscall_needs_before_insn(address_t pc, cpu_t cpu);
void syscall_before_insn(address_t pc, cpu_t cpu);

#endif // INTROSPECTION_H
