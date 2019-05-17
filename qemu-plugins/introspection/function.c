#include <stdint.h>
#include "plugins.h"
#include "regnum.h"
#include "introspection.h"

typedef struct Function {
    char *name;
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
    context_t ctx = vmi_get_context(cpu);
    Process *p = process_get(ctx);
    Function *f = g_hash_table_lookup(p->functions, &pc);
    qemulib_log("%llx: function %llx:%s\n", ctx, pc, f->name);
}

void function_add(context_t ctx, address_t entry, const char *name)
{
    Process *p = process_get(ctx);
    gint64 *key = g_new(gint64, 1);
    *key = entry;
    Function *f = g_new0(Function, 1);
    f->name = g_strdup(name);
    g_hash_table_insert(p->functions, key, f);
}
