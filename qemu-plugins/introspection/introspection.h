#ifndef INTROSPECTION_H
#define INTROSPECTION_H

#include <stdint.h>

typedef void * cpu_t;

/* Execution context */
typedef uint64_t context_t;
context_t vmi_get_context(cpu_t cpu);
uint64_t vmi_get_stack_pointer(cpu_t cpu);

#endif // INTROSPECTION_H
