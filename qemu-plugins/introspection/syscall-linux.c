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
        DPRINTF("%08x: trying to open file: %s\n", (int)ctx, filename);
        return p;
    }
    case 6: // sys_close
    {
        handle_t *h = g_new(handle_t, 1);
        *h = param1;
        DPRINTF("%08x: trying to close handle: %x\n", (int)ctx, (int)*h);
        return h;
    }
    case 8: // sys_create
    {
        char *filename = vmi_strdup(cpu, param1, 0);
        FileParams *p = g_new0(FileParams, 1);
        p->name = filename;
        DPRINTF("%08x: trying to create file: %s\n", (int)ctx, filename);
        return p;
    }
    case 90: // sys_mmap
    {
        DPRINTF("%08x: TODO: trying to mmap\n", (int)ctx);
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
            DPRINTF("%08x: trying to mmap2 %s\n", (int)ctx, file->filename);
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
        DPRINTF("%08x: trying to openat file: %s\n", (int)ctx, filename);
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
            DPRINTF("%08x: file: %x = %s\n", (int)ctx, (int)handle, p->name);
            file_open(vmi_get_context(cpu), p->name, handle);
            /* Don't free p->name */
        } else {
            DPRINTF("%08x: failed to open/create file: %s\n", (int)ctx, p->name);
            g_free(p->name);
        }
        break;
    }
    case 6: // sys_close
        if (!retval) {
            handle_t *h = sc->params;
            DPRINTF("%08x: close handle: %x\n", (int)ctx, (int)*h);
            file_close(vmi_get_context(cpu), *h);
        } else {
            DPRINTF("%08x: failed close handle\n", (int)ctx);
        }
        break;
    case 192: // sys_mmap2
    {
        MapFileParams *p = sc->params;
        if (retval != -1UL) {
            DPRINTF("%08x: mmap2 %x:%s\n", (int)ctx, (int)retval, p->file->filename);
            mapping_create(ctx, p->file->filename, retval, p->size);
        } else {
            DPRINTF("%08x: failed to mmap2 of the file: %s\n", (int)ctx, p->file->filename);
        }
        break;
    }
    }
    g_free(sc->params);
    sc->params = NULL;
}
