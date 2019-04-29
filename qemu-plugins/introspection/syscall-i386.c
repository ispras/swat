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

bool is_syscall_i386(address_t pc, cpu_t cpu)
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
    } else if (code == 0xcf) {
        /* also track iret for Linux */
        if (os_type == OS_LINUX) {
            return true;
        }
    } else if (code == 0xcd) {
        if (qemulib_read_memory(cpu, pc + 1, &code, 1)) {
            return false;
        }
        if (code == 0x80) {
            /* int 80 */
            return true;
        }
    }
    return false;
}

void syscall_i386(address_t pc, cpu_t cpu)
{
    context_t ctx = vmi_get_context(cpu);
    uint8_t code = 0;
    qemulib_read_memory(cpu, pc, &code, 1);
    /* iret is the first */
    if (code == 0xcf) {
        uint32_t esp = vmi_get_register(cpu, I386_ESP_REGNUM);
        /* Check nt_mask */
        if (vmi_get_register(cpu, I386_EFLAGS_REGNUM) & 0x00004000) {
            DPRINTF("TODO: NT mask is on, use another iret algorithm\n");
        } else {
            SCData *sc = sc_find(ctx, esp);
            /* get new ESP from the stack - try for sysenter-iret pair */
            if (!sc) {
                address_t addr = esp + vmi_get_register(cpu, I386_SS_BASE_REGNUM) + 12;
                esp = vmi_read_dword(cpu, addr);
                sc = sc_find(ctx, esp);
            }
            if (sc) {
                syscall_exit_linux(sc, pc, cpu);
                sc_erase(ctx, esp);
            }
        }
        return;
    }
    /* int 80 */
    if (code == 0xcd) {
        uint32_t reg = vmi_get_register(cpu, I386_EAX_REGNUM);
        address_t tr = vmi_get_register(cpu, I386_TR_BASE_REGNUM);
        uint32_t esp = vmi_read_dword(cpu, tr + 4) - 20;
        DPRINTF("%llx: int80 %x sp=%x\n", ctx, reg, esp);
        void *params = syscall_enter_linux(reg, pc, cpu);
        if (params) {
            sc_insert(ctx, esp, reg, params);
        }
        return;
    }
    qemulib_read_memory(cpu, pc + 1, &code, 1);
    /* log system calls */
    if (code == 0x34) {
        /* Read EAX which contains system call ID */
        uint32_t reg = vmi_get_register(cpu, I386_EAX_REGNUM);
        DPRINTF("%llx: sysenter %x\n", ctx, reg);
        void *params = NULL;
        if (os_type == OS_WINXP) {
            params = syscall_enter_winxp(reg, pc, cpu);
        } else if (os_type == OS_LINUX) {
            params = syscall_enter_linux(reg, pc, cpu);
        }
        if (params) {
            sc_insert(ctx, vmi_get_stack_pointer(cpu), reg, params);
        }
    } else if (code == 0x35) {
        uint32_t ecx = vmi_get_register(cpu, I386_ECX_REGNUM);
        SCData *sc = sc_find(ctx, ecx);
        if (sc) {
            // DPRINTF("%llx: sysexit %x\n", vmi_get_context(cpu), sc->num);
            if (os_type == OS_WINXP) {
                syscall_exit_winxp(sc, pc, cpu);
            } else if (os_type == OS_LINUX) {
                syscall_exit_linux(sc, pc, cpu);
            }
            sc_erase(ctx, ecx);
        }
    }
}
