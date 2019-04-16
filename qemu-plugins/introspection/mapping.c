#include "introspection.h"
#include "plugins.h"

static void mapping_destroy(gpointer data)
{
    Mapping *m = data;
    module_delete(m->module);
    g_free(m->filename);
    g_free(m);
}

static gint mapping_compare(gconstpointer a, gconstpointer b, gpointer opaque)
{
    const Mapping *k1 = a;
    const Mapping *k2 = b;
    if (k1->base < k2->base) {
        return -1;
    } else if (k1->base > k2->base) {
        return 1;
    } else if (k1->size < k2->size) {
        return -1;
    } else {
        return k1->size > k2->size;
    }
}

static gint mapping_search(gconstpointer a, gconstpointer b)
{
    const Mapping *k = a;
    address_t *addr = b;
    if (*addr < k->base) {
        return -1;
    } else if (*addr < k->base + k->size) {
        return 0;
    } else {
        return 1;
    }
}

void mapping_init_process(Process *p)
{
    g_assert(p);
    p->mappings = g_tree_new_full(mapping_compare,
        NULL, NULL, mapping_destroy);
}

void mapping_deinit_process(Process *p)
{
    g_assert(p && p->mappings);
    g_tree_destroy(p->mappings);
    p->mappings = NULL;
}

void mapping_create(context_t ctx, const char *filename,
                    address_t base, address_t size)
{
    Process *p = process_get(ctx);
    Mapping *m = g_new0(Mapping, 1);
    m->filename = g_strdup(filename);
    m->base = base;
    m->size = size;
    g_tree_insert(p->mappings, m, m);
}

static Mapping *mapping_find(context_t ctx, address_t addr)
{
    Process *p = process_get(ctx);
    return g_tree_search(p->mappings, mapping_search, &addr);
}

void mapping_delete(context_t ctx, address_t addr)
{
    Process *p = process_get(ctx);
    Mapping *m = mapping_find(ctx, addr);
    if (m) {
        g_tree_remove(p->mappings, m);
    }    
}
