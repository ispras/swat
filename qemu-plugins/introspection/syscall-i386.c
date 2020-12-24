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
    [5] = SYS_open,
    [6] = SYS_close,
    [8] = SYS_creat,
    [11] = SYS_execve,
    [192] = SYS_mmap,
    [295] = SYS_openat,
};

static const uint32_t winxp_syscall_map[] = {
    [0x19] = SYS_NtClose,
    [0x25] = SYS_NtCreateFile,
    [0x2f] = SYS_NtCreateProcess,
    [0x30] = SYS_NtCreateProcessEx,
    [0x32] = SYS_NtCreateSection,
    [0x7d] = SYS_NtOpenSection,
    [0x6c] = SYS_NtMapViewOfSection,
    [0x44] = SYS_NtDuplicateObject,
    [0x74] = SYS_NtOpenFile,
    [0x9a] = SYS_NtQueryInformationProcess,
    [0x10b] = SYS_NtUnmapViewOfSection,
};

#define GET_SYSCALL(map, id)                                           \
    (((id) < sizeof(map##_syscall_map) / sizeof(map##_syscall_map[0])) \
        ? map##_syscall_map[id]                                        \
        : SYS_Unknown)

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
                syscall_exit_linux32(sc, pc, cpu);
                sc_erase(ctx, esp);
            }
        }
        return;
    }
    /* int 80 */
    if (code == 0xcd) {
        uint32_t reg = vmi_get_register(cpu, I386_EAX_REGNUM);
        /* Only Linux uses int 0x80 */
        address_t tr = vmi_get_register(cpu, I386_TR_BASE_REGNUM);
        uint32_t esp = vmi_read_dword(cpu, tr + 4) - 20;
        DPRINTF("%llx: int80 %x sp=%x\n", ctx, reg, esp);
        reg = GET_SYSCALL(linux, reg);
        if (reg != SYS_Unknown) {
            void *params = syscall_enter_linux32(reg, pc, cpu);
            if (params) {
                sc_insert(ctx, esp, reg, params);
            }
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
            reg = GET_SYSCALL(winxp, reg);
            if (reg != SYS_Unknown) {
                params = syscall_enter_winxp(reg, pc, cpu);
            }
        } else if (os_type == OS_LINUX) {
            reg = GET_SYSCALL(linux, reg);
            if (reg != SYS_Unknown) {
                params = syscall_enter_linux32(reg, pc, cpu);
            }
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
                syscall_exit_linux32(sc, pc, cpu);
            }
            sc_erase(ctx, ecx);
        }
    }
}
