#include <glib.h>
#include "syscalls.h"
#include "plugins.h"
#include "regnum.h"

#define DEBUG
#ifdef DEBUG
#define DPRINTF qemulib_log
#else
static inline void DPRINTF(const char *fmt, ...) {}
#endif

bool is_syscall_x86_64(address_t pc, cpu_t cpu)
{
    uint8_t code = 0;
    if (!qemulib_read_memory(cpu, pc, &code, 1)) {
        if (code == 0x0f) {
            if (qemulib_read_memory(cpu, pc + 1, &code, 1)) {
                return false;
            }
            if (code == 0x05) {
                /* syscall */
                return true;
            }
        } else if (code == 0x48) {
            if (qemulib_read_memory(cpu, pc + 1, &code, 1)) {
                return false;
            }
            if (code != 0x0f) {
                return false;
            }
            if (qemulib_read_memory(cpu, pc + 2, &code, 1)) {
                return false;
            }
            if (code == 0x07) {
                /* sysret */
                return true;
            }
        }
    } 

    return false;
}

void syscall_x86_64(address_t pc, cpu_t cpu)
{
    context_t ctx = vmi_get_context(cpu);
    uint8_t code = 0;
    qemulib_read_memory(cpu, pc + 1, &code, 1);
    if (code == 0x05) {
        /* syscall */
        uint32_t reg = vmi_get_register(cpu, AMD64_RAX_REGNUM);
        DPRINTF("%llx: syscall %x\n", ctx, reg);
        void *params = NULL;
        if (os_type == OS_WIN10x64) {
            //params = syscall_enter_win10x64(reg, pc, cpu);
        }/* else if (os_type == OS_LINUX) {
            params = syscall_enter_linux(reg, pc, cpu);
        }*/
        /*if (params) {
            sc_insert(ctx, vmi_get_stack_pointer(cpu), reg, params);
        }*/
    } else {
        /* sysret */
        uint64_t rcx = vmi_get_register(cpu, AMD64_RCX_REGNUM);
        SCData *sc = sc_find(ctx, rcx);
        if (sc) {
            DPRINTF("%llx: sysret %x\n", vmi_get_context(cpu), sc->num);
            if (os_type == OS_WIN10x64) {
                //syscall_exit_win10x64(sc, pc, cpu);
            }/* else if (os_type == OS_LINUX) {
                syscall_exit_linux(sc, pc, cpu);
            }*/
            sc_erase(ctx, rcx);
        }
    }
}
