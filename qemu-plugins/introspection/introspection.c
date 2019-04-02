#include <stdint.h>
#include "plugins.h"
#include "introspection.h"

bool plugin_init(const char *args)
{
    process_init();
    section_init();

    return true;
}

bool plugin_needs_before_insn(uint64_t pc, void *cpu)
{
	return syscall_needs_before_insn(pc, cpu);
}

void plugin_before_insn(uint64_t pc, void *cpu)
{
    if (syscall_needs_before_insn(pc, cpu)) {
        syscall_before_insn(pc, cpu);
    }
}
