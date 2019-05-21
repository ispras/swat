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

/*typedef enum HandleType {
    H_SECTION,
    H_PROCESS,
} HandleType;*/

typedef struct Process {
    context_t context;
    guest_pid_t pid;

    /* List of the open files */
    GHashTable *files;
    /* List of the executing system calls */
    GHashTable *syscalls;
    /* List of the file mappings */
    GTree *mappings;
    /* List of the functions */
    GHashTable *functions;
    
    /* Platform-specific handles */
    /* List of the open sections */
    GHashTable *sectionHandles;
} Process;

typedef struct File {
    handle_t handle;
    char *filename;
} File;

typedef struct Section {
    char *name;
    char *filename;
} Section;

typedef struct Module Module;

typedef struct Mapping {
    char *filename;
    address_t base;
    address_t size;

    Module *module;
} Mapping;

/*typedef struct Handle {
    HandleType type;
    handle_t value;
    void *opaque;
} Handle;*/

/* Guest data extraction helpers */
uint64_t vmi_get_register(cpu_t cpu, int reg);
uint8_t vmi_read_byte(cpu_t cpu, address_t addr);
uint16_t vmi_read_word(cpu_t cpu, address_t addr);
uint32_t vmi_read_dword(cpu_t cpu, address_t addr);
char *vmi_strdup(cpu_t cpu, address_t addr, address_t maxlen);
/* Reads unicode string from the guest and converts it to utf-8 */
char *vmi_strdupw(cpu_t cpu, address_t addr, address_t maxlen);

/* Execution context */
context_t vmi_get_context(cpu_t cpu);
address_t vmi_get_stack_pointer(cpu_t cpu);

/* System calls */
bool syscall_init(const char *os_name);
void syscall_init_process(Process *p);
void syscall_deinit_process(Process *p);
bool syscall_needs_before_insn(address_t pc, cpu_t cpu);
void syscall_before_insn(address_t pc, cpu_t cpu);

/* File monitoring */
void file_init_process(Process *p);
void file_deinit_process(Process *p);
void file_open(context_t ctx, char *name, handle_t handle);
void file_close(context_t ctx, handle_t handle);
File *file_find(context_t ctx, handle_t handle);

/* Process information and monitoring */
void process_init(void);
Process *process_get(context_t ctx);

/* Windows: section monitoring */
void section_init(void);
void section_init_process(Process *p);
void section_deinit_process(Process *p);
void section_create(context_t ctx, char *name, handle_t handle, File *file);
Section *section_open(context_t ctx, char *name, handle_t handle);
void section_close(context_t ctx, handle_t handle);
Section *section_find(context_t ctx, handle_t handle);

/* File mappings */
void mapping_init_process(Process *p);
void mapping_deinit_process(Process *p);
void mapping_create(context_t ctx, const char *filename,
                    address_t base, address_t size);
void mapping_delete(context_t ctx, address_t addr);
Mapping *mapping_find(context_t ctx, address_t addr);

/* Modules */
void module_delete(Module *module);
bool module_needs_before_tb(address_t pc, cpu_t cpu);
void module_before_tb(address_t pc, cpu_t cpu);

/* Functions */
void function_init_process(Process *p);
void function_deinit_process(Process *p);
void function_add(context_t ctx, address_t entry, const char *name);
bool function_needs_before_insn(address_t pc, cpu_t cpu);
void function_before_insn(address_t pc, cpu_t cpu);

#endif // INTROSPECTION_H
