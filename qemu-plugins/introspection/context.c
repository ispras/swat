#include "introspection.h"
#include "plugins.h"
#include "regnum.h"

context_t vmi_get_context(cpu_t cpu)
{
    return vmi_get_register(cpu, I386_CR3_REGNUM);
}

address_t vmi_get_stack_pointer(cpu_t cpu)
{
    return vmi_get_register(cpu, I386_ESP_REGNUM);
}
