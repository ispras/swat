#include <stdint.h>
#include <glib.h>
#include "introspection.h"
#include "plugins.h"
#include "regnum.h"

typedef struct SCKey {
    context_t ctx;
    address_t sp;
} SCKey;

typedef struct SCData {
    uint32_t num;
} SCData;

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

static SCData *sc_find(context_t ctx, address_t sp)
{
    SCKey k = { .ctx = ctx, .sp = sp };
    return g_hash_table_lookup(syscalls, &k);
}

static void sc_erase(context_t ctx, address_t sp)
{
    SCKey k = { .ctx = ctx, .sp = sp };
    g_hash_table_remove(syscalls, &k);
}

static void sc_insert(context_t ctx, address_t sp, uint32_t num, void *param)
{
    SCKey *k = g_new(SCKey, 1);
    SCData *v = g_new(SCData, 1);
    k->ctx = ctx;
    k->sp = sp;
    v->num = num;

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
    uint8_t code = 0;
    if (!qemulib_read_memory(cpu, pc, &code, 1)
        && code == 0x0f) {
        if (qemulib_read_memory(cpu, pc + 1, &code, 1)) {
            return false;
        }
        if (code == 0x34) {
            /* sysenter */
            return true;
        }
        if (code == 0x35) {
            /* sysexit */
            return true;
        }
    }
    return false;
}

void syscall_before_insn(address_t pc, cpu_t cpu)
{
    uint8_t code = 0;
    uint32_t reg, ecx;
    qemulib_read_memory(cpu, pc + 1, &code, 1);
    /* Read EAX which contains system call ID */
    qemulib_read_register(cpu, (uint8_t*)&reg, I386_EAX_REGNUM);
    qemulib_read_register(cpu, (uint8_t*)&ecx, I386_ECX_REGNUM);
    /* log system calls */
    if (code == 0x34) {
        sc_insert(vmi_get_context(cpu), vmi_get_stack_pointer(cpu), reg, 0);
        qemulib_log("%llx: sysenter %x\n", vmi_get_context(cpu), reg);
    } else if (code == 0x35) {
        SCData *sc = sc_find(vmi_get_context(cpu), ecx);
        qemulib_log("%llx: sysexit %x ret=%x\n", vmi_get_context(cpu),
            sc ? sc->num : -1, reg);
        sc_erase(vmi_get_context(cpu), ecx);
    }
}
