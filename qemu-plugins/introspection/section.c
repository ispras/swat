#include "introspection.h"
#include "plugins.h"

/*
 * Global list of the sections.
 * They are only created and never destroyed.
 */
static GHashTable *sections;

static void section_destroy(gpointer data)
{
    Section *s = data;
    g_free(s->name);
    g_free(s->filename);
    g_free(s);
}

void section_init(void)
{
    sections = g_hash_table_new_full(g_str_hash, g_str_equal,
        NULL, section_destroy);
}

void section_init_process(Process *p)
{
    g_assert(p);
    p->sectionHandles = g_hash_table_new_full(g_int64_hash, g_int64_equal,
        g_free, NULL);
}

void section_deinit_process(Process *p)
{
    g_assert(p && p->sectionHandles);
    g_hash_table_destroy(p->sectionHandles);
    p->sectionHandles = NULL;
}

void section_create(context_t ctx, char *name, handle_t handle, File *file)
{
    Process *p = process_get(ctx);
    handle_t *key = g_new(handle_t, 1);
    *key = handle;
    Section *s = g_new(Section, 1);
    s->filename = g_strdup(file->filename);
    s->name = name;
    g_hash_table_insert(p->sectionHandles, key, s);
    if (s->name && s->name[0]) {
        g_hash_table_insert(sections, s->name, s);
    }
}

Section *section_open(context_t ctx, char *name, handle_t handle)
{
    g_assert(name && name[0]);
    // find the section
    Section *s = g_hash_table_lookup(sections, name);
    if (!s) {
        return NULL;
    }
    // add the handle
    Process *p = process_get(ctx);
    handle_t *key = g_new(handle_t, 1);
    *key = handle;
    g_hash_table_insert(p->sectionHandles, key, s);

    return s;
}

void section_close(context_t ctx, handle_t handle)
{
    Process *p = process_get(ctx);
    // TODO: close sections without the names
    g_hash_table_remove(p->sectionHandles, &handle);
}

Section *section_find(context_t ctx, handle_t handle)
{
    return g_hash_table_lookup(process_get(ctx)->sectionHandles, &handle);
}
