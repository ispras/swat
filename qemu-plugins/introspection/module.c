#include <stdint.h>
#include "plugins.h"
#include "module.h"


void module_delete(Module *module)
{
    if (module) {
        g_free(module->opaque);
        g_free(module);
    }
}


bool module_needs_before_tb(address_t pc, cpu_t cpu)
{
    return false;
}

void module_before_tb(address_t pc, cpu_t cpu)
{
}
