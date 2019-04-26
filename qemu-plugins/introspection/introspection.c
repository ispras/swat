#include <stdint.h>
#include "plugins.h"
#include "introspection.h"

bool plugin_init(const char *args)
{
    if (!syscall_init(args)) {
        return false;
    }

    process_init();
    section_init();

    return true;
}

bool plugin_needs_before_insn(uint64_t pc, void *cpu)
{
    return syscall_needs_before_insn(pc, cpu)
        || function_needs_before_insn(pc, cpu);
}

void plugin_before_insn(uint64_t pc, void *cpu)
{
    if (syscall_needs_before_insn(pc, cpu)) {
        syscall_before_insn(pc, cpu);
    }
    if (function_needs_before_insn(pc, cpu)) {
        function_before_insn(pc, cpu);
    }
}

bool plugin_needs_before_tb(uint64_t pc, void *cpu)
{
    return module_needs_before_tb(pc, cpu);
}

void plugin_before_tb(uint64_t pc, void *cpu)
{
    if (module_needs_before_tb(pc, cpu)) {
        module_before_tb(pc, cpu);
    }
}
