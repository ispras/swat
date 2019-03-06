#ifndef INTROSPECTION_H
#define INTROSPECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include <glib.h>

typedef void * cpu_t;
typedef uint64_t address_t;
typedef uint64_t context_t;
typedef uint64_t handle_t;
typedef uint64_t guest_pid_t;

typedef enum HandleType {
    H_FILE,
    H_PROCESS,
} HandleType;

typedef struct Process {
    context_t context;
    guest_pid_t pid;

    /* List of the open files */
    GHashTable *files;
    /* List of the executing system calls */
    GHashTable *syscalls;
} Process;

typedef struct File {
	handle_t handle;
	wchar_t *filename;
} File;

typedef struct Handle {
    HandleType type;
    handle_t value;
    void *opaque;
} Handle;

/* Guest data extraction helpers */
uint64_t vmi_get_register(cpu_t cpu, int reg);
uint16_t vmi_read_word(cpu_t cpu, address_t addr);
uint32_t vmi_read_dword(cpu_t cpu, address_t addr);
char *vmi_strdup(cpu_t cpu, address_t addr, address_t maxlen);
wchar_t *vmi_strdupw(cpu_t cpu, address_t addr, address_t maxlen);
wchar_t *strdupw(wchar_t *s);

/* Execution context */
context_t vmi_get_context(cpu_t cpu);
address_t vmi_get_stack_pointer(cpu_t cpu);

/* System calls */
void syscall_init_process(Process *p);
void syscall_deinit_process(Process *p);
bool syscall_needs_before_insn(address_t pc, cpu_t cpu);
void syscall_before_insn(address_t pc, cpu_t cpu);

/* File monitoring */
void file_init_process(Process *p);
void file_deinit_process(Process *p);
void file_open(context_t ctx, wchar_t *name, handle_t handle);
void file_close(context_t ctx, handle_t handle);
File *file_find(context_t ctx, handle_t handle);

/* Process information and monitoring */
void process_init(void);
Process *process_get(context_t ctx);

#endif // INTROSPECTION_H
