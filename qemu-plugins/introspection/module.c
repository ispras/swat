#include <stdint.h>
#include "plugins.h"
#include "module.h"
#include "syscalls.h"

typedef enum ModuleStatus {
    MS_UNKNOWN,
    MS_PARSED,
} ModuleStatus;

/* Executable module */
typedef struct Module {
    Mapping *mapping;
    ModuleStatus status;
} Module;

void module_delete(Module *module)
{
    g_free(module);
}

static void module_init(Mapping *m)
{
    Module *module = g_new(Module, 1);
    module->mapping = m;
    module->status = MS_UNKNOWN;
    m->module = module;
}

bool module_needs_before_tb(address_t pc, cpu_t cpu)
{
    context_t ctx = vmi_get_context(cpu);
    Mapping *m = mapping_find(ctx, pc);
    return m != NULL;
}

void module_before_tb(address_t pc, cpu_t cpu)
{
    context_t ctx = vmi_get_context(cpu);
    Process *p = process_get(ctx);
    Mapping *m = mapping_find(ctx, pc);
    if (!m->module) {
        module_init(m);
    }
    if (m->module->status == MS_UNKNOWN) {
        qemulib_log("Parsing module %s\n", m->filename);
        if (os_type == OS_WINXP) {
            if (parse_header_pe(cpu, m)) {
                m->module->status = MS_PARSED;
            }
        } else if (os_type == OS_LINUX) {
            if (parse_header_elf(cpu, m)) {
                m->module->status = MS_PARSED;
            }
        } else {
            m->module->status = MS_PARSED;
        }
    }
}
