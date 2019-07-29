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


void *syscall_enter_linux(uint32_t sc, address_t pc, cpu_t cpu)
{
    void *params = NULL;
    context_t ctx = vmi_get_context(cpu);
    uint32_t param1 = vmi_get_register(cpu, I386_EBX_REGNUM);
    uint32_t param2 = vmi_get_register(cpu, I386_ECX_REGNUM);
    uint32_t param3 = vmi_get_register(cpu, I386_EDX_REGNUM);
    uint32_t param4 = vmi_get_register(cpu, I386_ESI_REGNUM);
    uint32_t param5 = vmi_get_register(cpu, I386_EDI_REGNUM);
    uint32_t param6 = vmi_get_register(cpu, I386_EBP_REGNUM);
    switch (sc)
    {
    case 5: // sys_open
    {
        char *filename = vmi_strdup(cpu, param1, 0);
        FileParams *p = g_new0(FileParams, 1);
        p->name = filename;
        DPRINTF("%llx: trying to open file: %s\n", ctx, filename);
        return p;
    }
    case 6: // sys_close
    {
        handle_t *h = g_new(handle_t, 1);
        *h = param1;
        DPRINTF("%llx: trying to close handle: %x\n", ctx, (int)*h);
        return h;
    }
    case 8: // sys_create
    {
        char *filename = vmi_strdup(cpu, param1, 0);
        FileParams *p = g_new0(FileParams, 1);
        p->name = filename;
        DPRINTF("%llx: trying to create file: %s\n", ctx, filename);
        return p;
    }
    case 90: // sys_mmap
    {
        DPRINTF("%llx: TODO: trying to mmap\n", ctx);
        return NULL;
    }
    case 102: // sys_socketcall
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
    }
    case 192: // sys_mmap2
    {
        File *file = file_find(ctx, param5);
        if (file) {
            MapFileParams *p = g_new0(MapFileParams, 1);
            p->file = file;
            p->base = param1;
            p->size = param2;
            p->offset = param6; // in pages
            DPRINTF("%llx: trying to mmap2 %s\n", ctx, file->filename);
            return p;
        } else {
            //DPRINTF("%08x: trying to mmap2 of the unknown file %x\n", (int)ctx, param5);
        }
        return NULL;
    }
    case 295: // sys_openat
    {
        char *filename = vmi_strdup(cpu, param2, 0);
        FileParams *p = g_new0(FileParams, 1);
        p->name = filename;
        DPRINTF("%llx: trying to openat file: %s\n", ctx, filename);
        return p;
    }
    }
    return params;
}

void syscall_exit_linux(SCData *sc, address_t pc, cpu_t cpu)
{
    uint32_t retval = vmi_get_register(cpu, I386_EAX_REGNUM);
    context_t ctx = vmi_get_context(cpu);
    switch (sc->num)
    {
    case 5: // sys_open
    case 8: // sys_create
    case 295: // sys_openat
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
    case 6: // sys_close
        if (!retval) {
            handle_t *h = sc->params;
            DPRINTF("%llx: close handle: %x\n", ctx, (int)*h);
            file_close(vmi_get_context(cpu), *h);
        } else {
            DPRINTF("%llx: failed close handle\n", ctx);
        }
        break;
    case 192: // sys_mmap2
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
