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

#define SOCKETCALL_SOCKET      1
#define SOCKETCALL_BIND        2
#define SOCKETCALL_CONNECT     3
#define SOCKETCALL_LISTEN      4
#define SOCKETCALL_ACCEPT      5
#define SOCKETCALL_GETSOCKNAME 6
#define SOCKETCALL_GETPEERNAME 7
#define SOCKETCALL_SOCKETPAIR  8
#define SOCKETCALL_SEND        9
#define SOCKETCALL_RECV        10
#define SOCKETCALL_SENDTO      11
#define SOCKETCALL_RECVFROM    12
#define SOCKETCALL_SHUTDOWN    13
#define SOCKETCALL_SETSOCKOPT  14
#define SOCKETCALL_GETSOCKOPT  15
#define SOCKETCALL_SENDMSG     16
#define SOCKETCALL_RECVMSG     17
#define SOCKETCALL_ACCEPT4     18
#define SOCKETCALL_RECVMMSG    19
#define SOCKETCALL_SENDMMSG    20

static const int i386_args[6] = {
    I386_EBX_REGNUM, I386_ECX_REGNUM, I386_EDX_REGNUM,
    I386_ESI_REGNUM, I386_EDI_REGNUM, I386_EBP_REGNUM,
};

static const int x86_64_args[6] = {
    AMD64_RBX_REGNUM, AMD64_RCX_REGNUM, AMD64_RDX_REGNUM,
    AMD64_RSI_REGNUM, AMD64_RDI_REGNUM, AMD64_RBP_REGNUM,
};

static const int arm_args[6] = {
    ARM_A1_REGNUM, ARM_A1_REGNUM + 1, ARM_A1_REGNUM + 2,
    ARM_A1_REGNUM + 3, ARM_A1_REGNUM + 4, ARM_A1_REGNUM + 5,
};

void *syscall_enter_linux32(uint32_t sc, address_t pc, cpu_t cpu)
{
    void *params = NULL;
    context_t ctx = vmi_get_context(cpu);
    const int *a = i386_args;
    if (vmi_get_arch_type() == ARCH_ARM) {
        a = arm_args;
    } else if (vmi_get_arch_type() == ARCH_X86_64) {
        a = x86_64_args;
    }
    uint32_t arg1 = vmi_get_register(cpu, a[0]);
    uint32_t arg2 = vmi_get_register(cpu, a[1]);
    uint32_t arg3 = vmi_get_register(cpu, a[2]);
    uint32_t arg4 = vmi_get_register(cpu, a[3]);
    uint32_t arg5 = vmi_get_register(cpu, a[4]);
    uint32_t arg6 = vmi_get_register(cpu, a[5]);
    switch (sc)
    {
    case SYS_open:
    {
        char *filename = vmi_strdup(cpu, arg1, 0);
        FileParams *p = g_new0(FileParams, 1);
        p->name = filename;
        DPRINTF("%llx: trying to open file: %s\n", ctx, filename);
        return p;
    }
    case SYS_close:
    {
        handle_t *h = g_new(handle_t, 1);
        *h = arg1;
        DPRINTF("%llx: trying to close handle: %x\n", ctx, (int)*h);
        return h;
    }
    case SYS_creat:
    {
        char *filename = vmi_strdup(cpu, arg1, 0);
        FileParams *p = g_new0(FileParams, 1);
        p->name = filename;
        DPRINTF("%llx: trying to create file: %s\n", ctx, filename);
        return p;
    }
    /*case 102: // sys_socketcall
    {
        switch (param1)
        {
            case SOCKETCALL_SENDMSG:
            {
                DPRINTF("%llx: sendmsg\n", ctx);
                uint32_t args = param2;
                //uint32_t sockfd = vmi_read_dword(cpu, args);
                uint32_t msg = vmi_read_dword(cpu, args + 4);
                //uint32_t flags = vmi_read_dword(cpu, args + 8);
                uint32_t iov = vmi_read_dword(cpu, msg + 8);
                uint32_t iovlen = vmi_read_dword(cpu, msg + 12);
                while (iovlen--) {
                    char *str = vmi_strdup(cpu, iov, 0);
                    DPRINTF("--- buf: %s\n", str);
                    g_free(str);
                    iov += 8;
                }
                break;
            }
            default:
                DPRINTF("%llx: TODO: socketcall %x\n", ctx, param1);
                break;
        }
        return NULL;
    }*/
    case SYS_mmap: // TODO?
    case SYS_mmap2:
    {
        File *file = file_find(ctx, arg5);
        if (file) {
            MapFileParams *p = g_new0(MapFileParams, 1);
            p->file = file;
            p->base = arg1;
            p->size = arg2;
            p->offset = arg6; // in pages
            DPRINTF("%llx: trying to mmap2 %s\n", ctx, file->filename);
            return p;
        } else {
            //DPRINTF("%08x: trying to mmap2 of the unknown file %x\n", (int)ctx, param5);
        }
        return NULL;
    }
    case SYS_openat:
    {
        char *filename = vmi_strdup(cpu, arg2, 0);
        FileParams *p = g_new0(FileParams, 1);
        p->name = filename;
        DPRINTF("%llx: trying to openat file: %s\n", ctx, filename);
        return p;
    }
    case SYS_execve:
    {
        char *filename = vmi_strdup(cpu, arg1, 0);
        DPRINTF("%llx: execve: %s\n", ctx, filename);
        g_free(filename);
        return NULL;
    }
    }
    return params;
}

void syscall_exit_linux32(SCData *sc, address_t pc, cpu_t cpu)
{
    uint32_t retval = vmi_get_register(cpu,
        vmi_get_arch_type() == ARCH_I386
            ? I386_EAX_REGNUM
            : ARM_A1_REGNUM);
    context_t ctx = vmi_get_context(cpu);
    switch (sc->num)
    {
    case SYS_open:
    case SYS_creat:
    case SYS_openat:
    {
        FileParams *p = sc->params;
        if ((int32_t)retval >= 0) {
            handle_t handle = retval;
            DPRINTF("%llx: file: %x = %s\n", ctx, (int)handle, p->name);
            file_open(vmi_get_context(cpu), p->name, handle);
            /* Don't free p->name */
        } else {
            DPRINTF("%llx: failed to open/create file: %s\n", ctx, p->name);
            g_free(p->name);
        }
        break;
    }
    case SYS_close:
        if (!retval) {
            handle_t *h = sc->params;
            DPRINTF("%llx: close handle: %x\n", ctx, (int)*h);
            file_close(vmi_get_context(cpu), *h);
        } else {
            DPRINTF("%llx: failed close handle\n", ctx);
        }
        break;
    case SYS_mmap:
    {
        MapFileParams *p = sc->params;
        if (retval != -1UL) {
            DPRINTF("%llx: mmap2 %x+%x:%x:%s\n",
                ctx, (int)retval, (int)p->offset * 4096, (int)p->size,
                p->file->filename);
            mapping_create(ctx, p->file->filename, retval, p->size);
        } else {
            DPRINTF("%llx: failed to mmap2 of the file: %s\n", ctx, p->file->filename);
        }
        break;
    }
    }
    g_free(sc->params);
    sc->params = NULL;
}
