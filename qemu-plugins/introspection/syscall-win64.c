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

#define HANDLE_MASK (~3ULL)

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
    context_t ctx = vmi_get_context(cpu);
    uint64_t arg1 = vmi_get_register(cpu, AMD64_R10_REGNUM);
    uint64_t arg2 = vmi_get_register(cpu, AMD64_RDX_REGNUM);
    uint64_t arg3 = vmi_get_register(cpu, AMD64_R8_REGNUM);
    uint64_t arg4 = vmi_get_register(cpu, AMD64_R9_REGNUM);
    uint64_t arg5 = vmi_read_qword(cpu, vmi_get_stack_pointer(cpu) + 0x28);
    uint64_t arg6 = vmi_read_qword(cpu, vmi_get_stack_pointer(cpu) + 0x30);
    uint64_t arg7 = vmi_read_qword(cpu, vmi_get_stack_pointer(cpu) + 0x38);
    // uint64_t stack3 = vmi_read_qword(cpu, vmi_get_stack_pointer(cpu) + 0x40);
    DPRINTF("args: %llx %llx %llx %llx %llx %llx %llx\n", arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    switch (sc) {
    case SYS_NtClose:
    {
        handle_t *h = g_new(handle_t, 1);
        *h = arg1 & HANDLE_MASK;
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
    case SYS_NtCreateSection:
    {
        address_t attributes = arg3;
        address_t name = vmi_read_qword(cpu, attributes + 16);
        uint16_t len = vmi_read_word(cpu, name);
        address_t buf = vmi_read_qword(cpu, name + 8);
        char *str = vmi_strdupw(cpu, buf, len);

        address_t hfile = arg7 & HANDLE_MASK;
        File *file = file_find(ctx, hfile);
        if (file) {
            SectionParams *p = g_new(SectionParams, 1);
            p->phandle = arg1;
            p->name = str ? g_utf8_strup(str, -1) : NULL;
            g_free(str);
            p->file = file;
            return p;
        } else {
            DPRINTF("section with unknown file %x\n", hfile);
        }
        break;
    }
    case SYS_NtOpenSection:
    {
        address_t attributes = arg3;
        address_t name = vmi_read_qword(cpu, attributes + 16);
        uint16_t len = vmi_read_word(cpu, name);
        address_t buf = vmi_read_qword(cpu, name + 8);
        char *str = vmi_strdupw(cpu, buf, len);

        SectionParams *p = g_new0(SectionParams, 1);
        p->phandle = arg1;
        p->name = g_utf8_strup(str, -1);
        g_free(str);
        return p;
    }
    case SYS_NtMapViewOfSection:
    {
        handle_t sect = arg1 & HANDLE_MASK;
        handle_t proc = arg2;
        if (proc != 0xffffffffffffffff) {
            DPRINTF("unsupported map view\n");
        } else {
            Section *s = section_find(vmi_get_context(cpu), sect);
            if (!s) {
                DPRINTF("%llx: map view of unknown section %x\n",
                    ctx, sect);
                break;
            }
            MapSectionParams *p = g_new(MapSectionParams, 1);
            p->section = s;
            p->pbase = arg3;
            p->psize = arg7;
            return p;
        }
        break;
    }
    case SYS_NtUnmapViewOfSection:
    {
        handle_t handle = arg1;
        if (handle == 0xffffffffffffffff) {
            address_t *base = g_new(address_t, 1);
            *base = arg2;
            return base;
        } else {
            DPRINTF("unsupported section unmap for process %llx\n",
                handle);
        }
        break;
    }
    case SYS_NtDuplicateObject:
    {
        handle_t p1, p2;
        p1 = arg1;
        p2 = arg3;
        // current process
        if (p1 == 0xffffffffffffffff && p2 == p1) {
            address_t phandle = arg4;
            handle_t handle = arg2 & HANDLE_MASK;
            // TODO: find other handles
            File *f = file_find(ctx, handle);
            if (f) {
                DuplicateParams *p = g_new(DuplicateParams, 1);
                p->f = f;
                p->phandle = phandle;
                return p;
            } else {
                DPRINTF("unsupported duplicate object (unknown type)\n");
            }
        } else {
            DPRINTF("unsupported duplicate object (other process)\n");
        }
        break;
    }
    case SYS_NtQueryInformationProcess:
    {
        // IN HANDLE           ProcessHandle,
        // IN PROCESSINFOCLASS ProcessInformationClass,
        // OUT PVOID           ProcessInformation,
        // IN ULONG            ProcessInformationLength,
        // OUT PULONG          ReturnLength
        handle_t handle = arg1;
        uint32_t info = arg2;
        if (handle == 0xffffffffffffffff && info == 0) {
            address_t *a = g_new(address_t, 1);
            *a = arg3;
            return a;
        } else {
            DPRINTF("unsupported query information for handle %llx class %d\n",
                handle, info);
        }
        break;
    }
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
    case SYS_NtCreateSection:
    {
        SectionParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_qword(cpu, p->phandle);
            section_create(ctx, p->name, handle, p->file);
            DPRINTF("%llx: create section %llx:%s file:%s\n", ctx,
                handle, p->name, p->file->filename);
        } else {
            g_free(p->name);
        }
        break;
    }
    case SYS_NtOpenSection:
    {
        SectionParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_qword(cpu, p->phandle);
            Section *s = section_open(ctx, p->name, handle);
            if (s) {
                DPRINTF("%llx: open section %llx:%s file:%s\n",
                    ctx, handle, p->name, s->filename);
            } else {
                DPRINTF("%llx: internal error: no open section %s\n",
                    ctx, p->name);
            }
        } else {
            g_free(p->name);
        }
        break;
    }
    case SYS_NtMapViewOfSection:
    {
        MapSectionParams *p = sc->params;
        if (!retval) {
            address_t base = vmi_read_qword(cpu, p->pbase);
            address_t size = vmi_read_qword(cpu, p->psize);
            DPRINTF("%llx: map view %s/%s to %llx[%llx]\n", ctx,
                p->section->name, p->section->filename, base, size);
            mapping_create(ctx, p->section->filename, base, size);
        }
        break;
    }
    case SYS_NtUnmapViewOfSection:
        if (!retval) {
            address_t *base = sc->params;
            DPRINTF("%llx: unmap section base=%llx\n",
                ctx, *base);
            mapping_delete(ctx, *base);
        }
        break;
    case SYS_NtDuplicateObject:
    {
        DuplicateParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_qword(cpu, p->phandle);
            DPRINTF("%llx: duplicate %llx = %llx = %s\n", ctx,
                handle, p->f->handle, p->f->filename);
            file_open(vmi_get_context(cpu), g_strdup(p->f->filename), handle);
        }
        break;
    }
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
    {
        /* Current process only */
        address_t *addr = sc->params;
        if (!retval) {
            uint64_t pid = vmi_read_qword(cpu, *addr + 32);
            Process *p = process_get(ctx);
            p->pid = pid;
            DPRINTF("%llx: query pid %llx\n", ctx, pid);
        }
    }
    }
    g_free(sc->params);
    sc->params = NULL;
}
