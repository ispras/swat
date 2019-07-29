#include "introspection.h"
#include "plugins.h"
#include "regnum.h"

context_t vmi_get_context(cpu_t cpu)
{
    switch (vmi_get_arch_type()) {
    case ARCH_I386:
        return vmi_get_register(cpu, I386_CR3_REGNUM);
    case ARCH_X86_64:
        return vmi_get_register(cpu, AMD64_CR3_REGNUM);
    default:
        return 0;
    }
}

address_t vmi_get_stack_pointer(cpu_t cpu)
{
    switch (vmi_get_arch_type()) {
    case ARCH_I386:
        return vmi_get_register(cpu, I386_ESP_REGNUM);
    case ARCH_X86_64:
        return vmi_get_register(cpu, AMD64_RSP_REGNUM);
    default:
        return 0;
    }
}
