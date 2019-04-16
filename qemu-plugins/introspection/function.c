#include <stdint.h>
#include "plugins.h"
#include "introspection.h"

typedef struct Function {
    char nothing_here_yet;
} Function;

void function_init_process(Process *p)
{
    g_assert(p);
    p->functions = g_hash_table_new_full(g_int64_hash, g_int64_equal,
        g_free, g_free);
}

void function_deinit_process(Process *p)
{
    g_assert(p && p->functions);
    g_hash_table_destroy(p->functions);
    p->functions = NULL;
}

bool function_needs_before_insn(address_t pc, cpu_t cpu)
{
    Process *p = process_get(vmi_get_context(cpu));
    Function *f = g_hash_table_lookup(p->functions, &pc);
    return f != NULL;
}

void function_before_insn(address_t pc, cpu_t cpu)
{
    qemulib_log("Some function entry at %x\n", (int)pc);
}

void function_add(context_t ctx, address_t entry)
{
    Process *p = process_get(ctx);
    gint64 *key = g_new(gint64, 1);
    *key = entry;
    Function *f = g_new0(Function, 1);
    g_hash_table_insert(p->functions, key, f);
}
