#include <stdint.h>
#include <glib.h>
#include "introspection.h"
#include "plugins.h"
#include "regnum.h"
#include "syscalls.h"

SCData *sc_find(context_t ctx, address_t sp)
{
    Process *p = process_get(ctx);
    return g_hash_table_lookup(p->syscalls, &sp);
}

void sc_erase(context_t ctx, address_t sp)
{
    Process *p = process_get(ctx);
    g_hash_table_remove(p->syscalls, &sp);
}

void sc_insert(context_t ctx, address_t sp, uint32_t num, void *param)
{
    Process *p = process_get(ctx);
    address_t *k = g_new(address_t, 1);
    SCData *v = g_new(SCData, 1);
    *k = sp;
    v->num = num;
    v->params = param;

    if (!g_hash_table_insert(p->syscalls, k, v)) {
        qemulib_log("Overwriting syscall entry\n");
    }
}


void syscall_init_process(Process *p)
{
    p->syscalls = g_hash_table_new_full(g_int64_hash, g_int64_equal,
        g_free, g_free);
}

void syscall_deinit_process(Process *p)
{
    g_assert(p && p->syscalls);
    g_hash_table_destroy(p->syscalls);
    p->syscalls = NULL;
}

bool syscall_needs_before_insn(address_t pc, cpu_t cpu)
{
    return is_syscall_i386(pc, cpu);
}

void syscall_before_insn(address_t pc, cpu_t cpu)
{
    syscall_i386(pc, cpu);
}
