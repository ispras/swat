#include <glib.h>
#include "syscalls.h"
#include "plugins.h"
#include "regnum.h"
#include "syscall-functions.h"

#define DEBUG
#ifdef DEBUG
#define DPRINTF qemulib_log
#else
static inline void DPRINTF(const char *fmt, ...) {}
#endif

static const uint32_t linux_syscall_map[] = {
    [1024] = SYS_open,
    [57] = SYS_close,
    [1064] = SYS_creat,
    [222] = SYS_mmap,
    [56] = SYS_openat,
};

#undef S

#define GET_SYSCALL(map, id)                                           \
    (((id) < sizeof(map##_syscall_map) / sizeof(map##_syscall_map[0])) \
        ? map##_syscall_map[id]                                        \
        : SYS_Unknown)

static bool check_opcode(cpu_t cpu, address_t pc, uint32_t op)
{
    for (int i = 0 ; i < 32 ; i += 8, ++pc, op >>= 8) {
        uint8_t code = 0;
        if (qemulib_read_memory(cpu, pc, &code, 1)) {
            return false;
        }
        if (code != (op & 0xff)) {
            return false;
        }
    }
    return true;
}

bool is_syscall_aarch64(address_t pc, cpu_t cpu)
{
    if (check_opcode(cpu, pc, 0xd4000001)) {
        /* svc */
        return true;
    } else if (check_opcode(cpu, pc, 0xd69f03e0)) {
        /* eret */
        return true;
    }

    return false;
}

void syscall_aarch64(address_t pc, cpu_t cpu)
{
    context_t ctx = vmi_get_context(cpu);
    uint8_t code = 0;
    qemulib_read_memory(cpu, pc, &code, 1);
    if (code == 0x01) {
        /* svc */
        uint32_t id = vmi_get_register(cpu, AARCH64_X0_REGNUM + 8);
        DPRINTF("%llx: syscall %d\n", ctx, id);
        void *params = NULL;
        id = GET_SYSCALL(linux, id);
        if (id != SYS_Unknown) {
            params = syscall_enter_linux64(id, pc, cpu);
        }
        if (params) {
            sc_insert(ctx, vmi_get_stack_pointer(cpu), id, params);
        }
    } else {
        /* eret */
        uint64_t sp = vmi_get_register(cpu, AARCH64_SP_EL0);
        SCData *sc = sc_find(ctx, sp);
        if (sc) {
            //DPRINTF("%llx: sysret %x\n", vmi_get_context(cpu), sc->num);
            syscall_exit_linux64(sc, pc, cpu);
            sc_erase(ctx, sp);
        }
    }
}
