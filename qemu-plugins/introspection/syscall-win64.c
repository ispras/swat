#include <glib.h>
#include "syscalls.h"
#include "plugins.h"
#include "regnum.h"

#define DEBUG
#ifdef DEBUG
#define DPRINTF qemulib_log
#else
static inline void DPRINTF(const char *fmt, ...) {}
#endif

void *syscall_enter_win10x64(uint32_t sc, address_t pc, cpu_t cpu)
{
    void *params = NULL;
    params = g_new(int, 1);
    return params;
}

void syscall_exit_win10x64(SCData *sc, address_t pc, cpu_t cpu)
{
    g_free(sc->params);
    sc->params = NULL;
}
