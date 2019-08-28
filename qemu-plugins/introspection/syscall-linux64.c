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

void *syscall_enter_linux64(uint32_t sc, address_t pc, cpu_t cpu)
{
    void *params = NULL;
    context_t ctx = vmi_get_context(cpu);
    uint64_t arg1 = vmi_get_register(cpu, AMD64_RDI_REGNUM);
    uint64_t arg2 = vmi_get_register(cpu, AMD64_RSI_REGNUM);
    uint64_t arg3 = vmi_get_register(cpu, AMD64_RDX_REGNUM);
    uint64_t arg4 = vmi_get_register(cpu, AMD64_R10_REGNUM);
    uint64_t arg5 = vmi_get_register(cpu, AMD64_R8_REGNUM);
    uint64_t arg6 = vmi_get_register(cpu, AMD64_R9_REGNUM);
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
    case SYS_mmap:
    {
        File *file = file_find(ctx, arg5);
        if (file) {
            MapFileParams *p = g_new0(MapFileParams, 1);
            p->file = file;
            p->base = arg1;
            p->size = arg2;
            p->offset = arg6; // in pages
            DPRINTF("%llx: trying to mmap %s\n", ctx, file->filename);
            return p;
        } else {
            DPRINTF("%llx: trying to mmap of the unknown file %llx\n", ctx, arg5);
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
    }

    return params;
}

void syscall_exit_linux64(SCData *sc, address_t pc, cpu_t cpu)
{
    uint64_t retval = vmi_get_register(cpu, AMD64_RAX_REGNUM);
    context_t ctx = vmi_get_context(cpu);
    switch (sc->num)
    {
    case SYS_open:
    case SYS_creat:
    case SYS_openat:
    {
        FileParams *p = sc->params;
        if ((int64_t)retval >= 0) {
            handle_t handle = retval;
            DPRINTF("%llx: file: %llx = %s\n", ctx, handle, p->name);
            file_open(ctx, p->name, handle);
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
            DPRINTF("%llx: close handle: %llx\n", ctx, *h);
            file_close(vmi_get_context(cpu), *h);
        } else {
            DPRINTF("%llx: failed close handle\n", ctx);
        }
        break;
    case SYS_mmap:
    {
        MapFileParams *p = sc->params;
        if (retval != -1UL) {
            DPRINTF("%llx: mmap %llx+%llx:%llx:%s\n",
                ctx, retval, p->offset * 4096, p->size,
                p->file->filename);
            mapping_create(ctx, p->file->filename, retval, p->size);
        } else {
            DPRINTF("%llx: failed to mmap of the file: %s\n",
                ctx, p->file->filename);
        }
        break;
    }
    }
    g_free(sc->params);
    sc->params = NULL;
}
