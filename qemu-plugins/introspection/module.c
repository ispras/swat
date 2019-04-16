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
