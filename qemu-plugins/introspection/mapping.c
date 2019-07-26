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

static gint mapping_compare_name(gconstpointer a, gconstpointer b, gpointer opaque)
{
    return g_strcmp0(a, b);
}

static gint mapping_search(gconstpointer a, gconstpointer b)
{
    const Mapping *k = a;
    const address_t *addr = b;
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
    p->mappings_by_name = g_tree_new_full(mapping_compare_name,
        NULL, NULL, NULL);
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
    Mapping *m = g_tree_lookup(p->mappings_by_name, filename);
    if (m) {
        g_tree_steal(p->mappings, m);
        address_t old_base = m->base;
        address_t old_size = m->size;
        if (old_base > base) {
            m->base = base;
        }
        address_t old_end = old_base + old_size;
        if (old_end < base + size) {
            old_end = base + size;
        }
        m->size = old_end - m->base;
        g_tree_insert(p->mappings, m, m);
        qemulib_log("%lx: extending mapping %s to %llx:%llx\n",
            ctx, filename, m->base, m->size);
    } else {
        m = g_new0(Mapping, 1);
        m->filename = g_strdup(filename);
        m->base = base;
        m->size = size;
        g_tree_insert(p->mappings, m, m);
        g_tree_insert(p->mappings_by_name, m->filename, m);
    }
}

Mapping *mapping_find(context_t ctx, address_t addr)
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
