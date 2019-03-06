#include <stdint.h>
#include <glib.h>
#include "introspection.h"
#include "plugins.h"

static void file_destroy(gpointer data)
{
    File *f = data;
    g_free(f->filename);
    g_free(f);
}

void file_init_process(Process *p)
{
    g_assert(p);
    p->files = g_hash_table_new_full(g_int64_hash, g_int64_equal,
        g_free, file_destroy);
}

void file_deinit_process(Process *p)
{
    g_assert(p && p->files);
    g_hash_table_destroy(p->files);
    p->files = NULL;
}

void file_open(context_t ctx, char *name, handle_t handle)
{
    Process *p = process_get(ctx);
    handle_t *key = g_new(handle_t, 1);
    *key = handle;
    File *f = g_new(File, 1);
    f->handle = handle;
    f->filename = name;
    g_hash_table_insert(p->files, key, f);
}

void file_close(context_t ctx, handle_t handle)
{
    Process *p = process_get(ctx);
    g_hash_table_remove(p->files, &handle);
}

File *file_find(context_t ctx, handle_t handle)
{
    return g_hash_table_lookup(process_get(ctx)->files, &handle);
}
