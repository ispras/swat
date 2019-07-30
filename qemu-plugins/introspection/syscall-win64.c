#include <glib.h>
#include "syscalls.h"
#include "plugins.h"
#include "regnum.h"
#include "syscalls-windows.h"

#define DEBUG
#ifdef DEBUG
#define DPRINTF qemulib_log
#else
static inline void DPRINTF(const char *fmt, ...) {}
#endif

// static void print_buf(cpu_t cpu, address_t addr)
// {
//     int i = 0;
//     qemulib_log("buffer %llx:", addr);
//     for (i = 0 ; i < 32 ; ++i) {
//         qemulib_log(" %02x", vmi_read_byte(cpu, addr + i));
//     }
//     qemulib_log("\n");
// }

void *syscall_enter_win64(uint32_t sc, address_t pc, cpu_t cpu)
{
    void *params = NULL;
    uint64_t arg1 = vmi_get_register(cpu, AMD64_R10_REGNUM);
    uint64_t arg2 = vmi_get_register(cpu, AMD64_RDX_REGNUM);
    uint64_t arg3 = vmi_get_register(cpu, AMD64_R8_REGNUM);
    uint64_t arg4 = vmi_get_register(cpu, AMD64_R9_REGNUM);
    // uint64_t stack0 = vmi_read_qword(cpu, vmi_get_stack_pointer(cpu) + 0x28);
    // uint64_t stack1 = vmi_read_qword(cpu, vmi_get_stack_pointer(cpu) + 0x30);
    // uint64_t stack2 = vmi_read_qword(cpu, vmi_get_stack_pointer(cpu) + 0x38);
    // uint64_t stack3 = vmi_read_qword(cpu, vmi_get_stack_pointer(cpu) + 0x40);
    switch (sc) {
    case SYS_NtClose:
    {
        handle_t *h = g_new(handle_t, 1);
        *h = arg1;
        return h;
    }
    case SYS_NtOpenFile:
    case SYS_NtCreateFile:
    {
        address_t phandle = arg1;
        address_t attributes = arg3;
        address_t name = vmi_read_qword(cpu, attributes + 16);
        uint16_t len = vmi_read_word(cpu, name);
        address_t buf = vmi_read_qword(cpu, name + 8);
        char *str = vmi_strdupw(cpu, buf, len);
        FileParams *p = g_new(FileParams, 1);
        p->phandle = phandle;
        p->name = str;
        return p;
    }
    case SYS_NtCreateProcess:
        break;
    case SYS_NtCreateProcessEx:
        break;
    case SYS_NtCreateSection:
        break;
    case SYS_NtOpenSection:
        break;
    case SYS_NtMapViewOfSection:
        break;
    case SYS_NtDuplicateObject:
        break;
    case SYS_NtQueryInformationProcess:
        break;
    case SYS_NtUnmapViewOfSection:
        break;
    }
    return params;
}

void syscall_exit_win64(SCData *sc, address_t pc, cpu_t cpu)
{
    uint64_t retval = vmi_get_register(cpu, AMD64_RAX_REGNUM);
    context_t ctx = vmi_get_context(cpu);
    switch (sc->num) {
    case SYS_NtClose:
        if (!retval) {
            handle_t *h = sc->params;
            DPRINTF("%llx: close handle: %llx\n", ctx, *h);
            file_close(vmi_get_context(cpu), *h);
            section_close(vmi_get_context(cpu), *h);
        }
        break;
    case SYS_NtCreateProcess:
        break;
    case SYS_NtCreateProcessEx:
        break;
    case SYS_NtCreateSection:
        break;
    case SYS_NtOpenSection:
        break;
    case SYS_NtMapViewOfSection:
        break;
    case SYS_NtDuplicateObject:
        break;
    case SYS_NtCreateFile:
        DPRINTF("create\n");
        /* Fallthrough */
    case SYS_NtOpenFile:
    {
        FileParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_qword(cpu, p->phandle);
            DPRINTF("%llx: open file: %llx = %s\n", ctx, handle, p->name);
            file_open(ctx, p->name, handle);
            /* Don't free p->name */
        } else {
            g_free(p->name);
        }
        break;
    }
    case SYS_NtQueryInformationProcess:
        break;
    case SYS_NtUnmapViewOfSection:
        break;
    }
    g_free(sc->params);
    sc->params = NULL;
}
