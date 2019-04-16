#include "introspection.h"
#include "plugins.h"

static GHashTable *processes;

static void process_destroy(gpointer data)
{
    Process *p = data;
    function_deinit_process(p);
    mapping_deinit_process(p);
    section_deinit_process(p);
    file_deinit_process(p);
    syscall_deinit_process(p);
    g_free(p);
}

static Process *process_create(context_t ctx)
{
    Process *p = g_new0(Process, 1);
    p->context = ctx;
    syscall_init_process(p);
    file_init_process(p);
    section_init_process(p);
    mapping_init_process(p);
    function_init_process(p);

    context_t *key = g_new(context_t, 1);
    *key = ctx;
    g_hash_table_insert(processes, key, p);

    return p;
}

void process_init(void)
{
    processes = g_hash_table_new_full(g_int64_hash, g_int64_equal,
        g_free, process_destroy);
}


Process *process_get(context_t ctx)
{
    Process *p = g_hash_table_lookup(processes, &ctx);
    if (!p) {
        p = process_create(ctx);
    }
    return p;
}
