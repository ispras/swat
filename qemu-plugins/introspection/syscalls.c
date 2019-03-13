#include <stdint.h>
#include <glib.h>
#include "introspection.h"
#include "plugins.h"
#include "regnum.h"
#include "syscalls.h"

typedef struct SCKey {
    context_t ctx;
    address_t sp;
} SCKey;

static GHashTable *syscalls;

static guint sc_key_hash(gconstpointer k)
{
    const SCKey *key = k;
    uint64_t v = key->sp ^ key->ctx;
    return (guint)(v ^ (v >> 32));
}

static gboolean sc_key_equal(gconstpointer a, gconstpointer b)
{
    const SCKey *k1 = a;
    const SCKey *k2 = b;
    return k1->sp == k2->sp && k1->ctx == k2->ctx;
}

static void sc_value_destroy(gpointer data)
{
    g_free(data);
}

static void sc_init(void)
{
    syscalls = g_hash_table_new_full(sc_key_hash, sc_key_equal,
        g_free, sc_value_destroy);
}

SCData *sc_find(context_t ctx, address_t sp)
{
    SCKey k = { .ctx = ctx, .sp = sp };
    return g_hash_table_lookup(syscalls, &k);
}

void sc_erase(context_t ctx, address_t sp)
{
    SCKey k = { .ctx = ctx, .sp = sp };
    g_hash_table_remove(syscalls, &k);
}

void sc_insert(context_t ctx, address_t sp, uint32_t num, void *param)
{
    SCKey *k = g_new(SCKey, 1);
    SCData *v = g_new(SCData, 1);
    k->ctx = ctx;
    k->sp = sp;
    v->num = num;
    v->params = param;

    if (!g_hash_table_insert(syscalls, k, v)) {
        qemulib_log("Overwriting syscall entry\n");
    }
}


void syscall_init(void)
{
    sc_init();
}

bool syscall_needs_before_insn(address_t pc, cpu_t cpu)
{
    return is_syscall_i386(pc, cpu);
}

void syscall_before_insn(address_t pc, cpu_t cpu)
{
    syscall_i386(pc, cpu);
}
