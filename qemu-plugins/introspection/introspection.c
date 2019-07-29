#include <stdint.h>
#include "plugins.h"
#include "introspection.h"

static arch_type = ARCH_UNKNOWN;

bool plugin_init(const char *args)
{
    /* Check supported architectures */
    const char *arch = qemulib_get_arch_name();
    if (!strcmp(arch, "i386")) {
        arch_type = ARCH_I386;
    } else if (!strcmp(arch, "x86_64")) {
        arch_type = ARCH_X86_64;
    } else {
        return false;
    }

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

ArchType vmi_get_arch_type(void)
{
    return arch_type;
}
