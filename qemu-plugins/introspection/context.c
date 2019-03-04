#include "introspection.h"
#include "plugins.h"
#include "regnum.h"

context_t vmi_get_context(cpu_t cpu)
{
    uint32_t reg;
    qemulib_read_register(cpu, (uint8_t*)&reg, I386_CR3_REGNUM);
    return reg;
}

uint64_t vmi_get_stack_pointer(cpu_t cpu)
{
    uint32_t reg;
    qemulib_read_register(cpu, (uint8_t*)&reg, I386_ESP_REGNUM);
    return reg;
}
