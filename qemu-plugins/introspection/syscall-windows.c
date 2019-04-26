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

#define HANDLE_MASK (~3U)

void *syscall_enter_winxp(uint32_t sc, address_t pc, cpu_t cpu)
{
    void *params = NULL;
    address_t data = vmi_get_register(cpu, I386_EDX_REGNUM) + 8;
    context_t ctx = vmi_get_context(cpu);
    switch (sc)
    {
    case 0x19: // NtClose
    {
        // IN HANDLE Handle
        handle_t *h = g_new(handle_t, 1);
        *h = vmi_read_dword(cpu, data);
        return h;
    }
    case 0x25: // NtCreateFile
    {
        // OUT PHANDLE           FileHandle,
        // IN ACCESS_MASK        DesiredAccess,
        // IN POBJECT_ATTRIBUTES ObjectAttributes,
        // OUT PIO_STATUS_BLOCK  IoStatusBlock,
        // IN PLARGE_INTEGER     AllocationSize,
        // IN ULONG              FileAttributes,
        // IN ULONG              ShareAccess,
        // IN ULONG              CreateDisposition,
        // IN ULONG              CreateOptions,
        // IN PVOID              EaBuffer,
        // IN ULONG              EaLength
        address_t phandle = vmi_read_dword(cpu, data);
        address_t attributes = vmi_read_dword(cpu, data + 8);
        address_t name = vmi_read_dword(cpu, attributes + 8);
        uint16_t len = vmi_read_word(cpu, name);
        address_t buf = vmi_read_dword(cpu, name + 4);
        char *str = vmi_strdupw(cpu, buf, len);
        FileParams *p = g_new(FileParams, 1);
        p->phandle = phandle;
        p->name = str;
        return p;
    }
    case 0x2f: // NtCreateProcess
        // OUT PHANDLE           ProcessHandle,
        // IN ACCESS_MASK        DesiredAccess,
        // IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
        // IN HANDLE             ParentProcess,
        // IN BOOLEAN            InheritObjectTable,
        // IN HANDLE             SectionHandle OPTIONAL,
        // IN HANDLE             DebugPort OPTIONAL,
        // IN HANDLE             ExceptionPort OPTIONAL );
        /* Fallthrough */
    case 0x30: // NtCreateProcessEx
    {
        // OUT PHANDLE ProcessHandle,
        // IN ACCESS_MASK DesiredAccess,
        // IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
        // IN HANDLE ParentProcess,
        // IN ULONG Flags,
        // IN HANDLE SectionHandle OPTIONAL,
        // IN HANDLE DebugPort OPTIONAL,
        // IN HANDLE ExceptionPort OPTIONAL,
        // IN BOOLEAN InJob)
        break;
    }
    case 0x32: // NtCreateSection
    {
        // PHANDLE            SectionHandle,
        // ACCESS_MASK        DesiredAccess,
        // POBJECT_ATTRIBUTES ObjectAttributes,
        // PLARGE_INTEGER     MaximumSize,
        // ULONG              SectionPageProtection,
        // ULONG              AllocationAttributes,
        // HANDLE             FileHandle
        address_t attributes = vmi_read_dword(cpu, data + 8);
        address_t name = vmi_read_dword(cpu, attributes + 8);
        uint16_t len = vmi_read_word(cpu, name);
        address_t buf = vmi_read_dword(cpu, name + 4);
        char *str = vmi_strdupw(cpu, buf, len);

        address_t hfile = vmi_read_dword(cpu, data + 24) & HANDLE_MASK;
        File *file = file_find(ctx, hfile);
        if (file) {
            SectionParams *p = g_new(SectionParams, 1);
            p->phandle = vmi_read_dword(cpu, data);
            p->name = str ? g_utf8_strup(str, -1) : NULL;
            g_free(str);
            p->file = file;
            return p;
        } else {
            DPRINTF("section with unknown file %x\n", hfile);
        }
        break;
    }
    // TODO: NtCreateSectionEx
    case 0x7d: // NtOpenSection
    {
        // PHANDLE            SectionHandle,
        // ACCESS_MASK        DesiredAccess,
        // POBJECT_ATTRIBUTES ObjectAttributes
        address_t attributes = vmi_read_dword(cpu, data + 8);
        address_t name = vmi_read_dword(cpu, attributes + 8);
        uint16_t len = vmi_read_word(cpu, name);
        address_t buf = vmi_read_dword(cpu, name + 4);

        SectionParams *p = g_new0(SectionParams, 1);
        p->phandle = vmi_read_dword(cpu, data);
        char *str = vmi_strdupw(cpu, buf, len);
        p->name = g_utf8_strup(str, -1);
        g_free(str);
        return p;
    }
    case 0x6c: // NtMapViewOfSection
    {
        // HANDLE          SectionHandle,
        // HANDLE          ProcessHandle,
        // PVOID           *BaseAddress,
        // ULONG_PTR       ZeroBits,
        // SIZE_T          CommitSize,
        // PLARGE_INTEGER  SectionOffset,
        // PSIZE_T         ViewSize,
        // SECTION_INHERIT InheritDisposition,
        // ULONG           AllocationType,
        // ULONG           Win32Protect        

        uint32_t sect = vmi_read_dword(cpu, data) & HANDLE_MASK;
        uint32_t proc = vmi_read_dword(cpu, data + 4);
        if (proc != 0xffffffff) {
            DPRINTF("unsupported map view\n");
        } else {
            Section *s = section_find(vmi_get_context(cpu), sect);
            if (!s) {
                DPRINTF("%08x: map view of unknown section %x\n",
                    ctx, sect);
                break;
            }
            MapSectionParams *p = g_new(MapSectionParams, 1);
            p->section = s;
            p->pbase = vmi_read_dword(cpu, data + 8);
            p->psize = vmi_read_dword(cpu, data + 24);
            return p;
        }
        break;
    }
    case 0x44: // NtDuplicateObject
    {
        // TODO: can't identify the process by handle
        uint32_t p1, p2;
        p1 = vmi_read_dword(cpu, data);
        p2 = vmi_read_dword(cpu, data + 8);
        // current process
        if (p1 == 0xffffffff && p2 == p1) {
            address_t phandle = vmi_read_dword(cpu, data + 4);
            handle_t handle = vmi_read_dword(cpu, phandle) & HANDLE_MASK;
            address_t out = vmi_read_dword(cpu, data + 12);
            // TODO: find other handles
            File *f = file_find(vmi_get_context(cpu), handle);
            if (f) {
                DuplicateParams *p = g_new(DuplicateParams, 1);
                p->f = f;
                p->phandle = out;
                return p;
            } else {
                DPRINTF("unsupported duplicate object (unknown type)\n");
            }
        } else {
            DPRINTF("unsupported duplicate object (other process)\n");
        }
        // IN HANDLE               SourceProcessHandle,
        // IN PHANDLE              SourceHandle,
        // IN HANDLE               TargetProcessHandle,
        // OUT PHANDLE             TargetHandle,
        // IN ACCESS_MASK          DesiredAccess OPTIONAL,
        // IN BOOLEAN              InheritHandle,
        // IN ULONG                Options );
        break;
    }
    case 0x74: // NtOpenFile
    {
        // OUT PHANDLE           FileHandle,
        // IN ACCESS_MASK        DesiredAccess,
        // IN POBJECT_ATTRIBUTES ObjectAttributes,
        // OUT PIO_STATUS_BLOCK  IoStatusBlock,
        // IN ULONG              ShareAccess,
        // IN ULONG              OpenOptions
        address_t phandle = vmi_read_dword(cpu, data);
        address_t attributes = vmi_read_dword(cpu, data + 8);
        // ULONG           Length;
        // HANDLE          RootDirectory;
        // PUNICODE_STRING ObjectName;
        // ULONG           Attributes;
        // PVOID           SecurityDescriptor;
        // PVOID           SecurityQualityOfService;
        address_t name = vmi_read_dword(cpu, attributes + 8);
        // USHORT Length;
        // USHORT MaximumLength;
        // PWSTR  Buffer;
        uint16_t len = vmi_read_word(cpu, name);
        address_t buf = vmi_read_dword(cpu, name + 4);

        char *str = vmi_strdupw(cpu, buf, len);
        FileParams *p = g_new(FileParams, 1);
        p->phandle = phandle;
        p->name = str;
        return p;
    }
    case 0x9a: // NtQueryInformationProcess
    {
        // IN HANDLE           ProcessHandle,
        // IN PROCESSINFOCLASS ProcessInformationClass,
        // OUT PVOID           ProcessInformation,
        // IN ULONG            ProcessInformationLength,
        // OUT PULONG          ReturnLength
        handle_t handle = vmi_read_dword(cpu, data);
        uint32_t info = vmi_read_dword(cpu, data + 4);
        if (handle == 0xffffffff && info == 0) {
            address_t *a = g_new(address_t, 1);
            *a = vmi_read_dword(cpu, data + 8);
            return a;
        } else {
            DPRINTF("unsupported query information for handle %x class %d\n",
                (int)handle, info);
        }
    }
    case 0xb7: // NtReadFile
        break;
    case 0x10b: // NtUnmapViewOfSection
    {
        // HANDLE ProcessHandle,
        // PVOID  BaseAddress
        handle_t handle = vmi_read_dword(cpu, data);
        if (handle == 0xffffffff) {
            address_t *base = g_new(address_t, 1);
            *base = vmi_read_dword(cpu, data + 4);
            return base;
        } else {
            DPRINTF("unsupported section unmap for process %x\n",
                (int)handle);
        }
        break;
    }
    case 0x112: // NtWriteFile
        break;
    }
    return params;
}

void syscall_exit_winxp(SCData *sc, address_t pc, cpu_t cpu)
{
    uint32_t retval = vmi_get_register(cpu, I386_EAX_REGNUM);
    context_t ctx = vmi_get_context(cpu);
    switch (sc->num)
    {
    case 0x19: // NtClose
    {
        if (!retval) {
            handle_t *h = sc->params;
            DPRINTF("%08x: close handle: %x\n", (int)ctx, (int)*h);
            file_close(vmi_get_context(cpu), *h);
            section_close(vmi_get_context(cpu), *h);
        }
        break;
    }
    case 0x32: // NtCreateSection
    {
        SectionParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_dword(cpu, p->phandle);
            section_create(ctx, p->name, handle, p->file);
            DPRINTF("%08x: create section %x:%s file:%s\n", (int)ctx,
                (int)handle, p->name, p->file->filename);
        } else {
            g_free(p->name);
        }
        break;
    }
    case 0x44: // NtDuplicateObject
    {
        DuplicateParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_dword(cpu, p->phandle);
            DPRINTF("%08x: duplicate %x = %x = %ls\n", (int)ctx,
                (int)handle, (int)p->f->handle, p->f->filename);
            file_open(vmi_get_context(cpu), g_strdup(p->f->filename), handle);
        }
        break;
    }
    case 0x6c: // NtMapViewOfSection
    {
        MapSectionParams *p = sc->params;
        if (!retval) {
            address_t base = vmi_read_dword(cpu, p->pbase);
            address_t size = vmi_read_dword(cpu, p->psize);
            DPRINTF("%08x: map view %s/%s to %x[%x]\n", (int)ctx,
                p->section->name, p->section->filename, (int)base, (int)size);
            mapping_create(ctx, p->section->filename, base, size);
        }
        break;
    }
    case 0x25: // NtCreateFile
        DPRINTF("create\n");
        /* Fallthrough */
    case 0x74: // NtOpenFile
    {
        FileParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_dword(cpu, p->phandle);
            DPRINTF("%08x: open file: %x = %s\n", (int)ctx, (int)handle, p->name);
            file_open(vmi_get_context(cpu), p->name, handle);
            /* Don't free p->name */
        } else {
            g_free(p->name);
        }
        break;
    }
    case 0x7d: // NtOpenSection
    {
        SectionParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_dword(cpu, p->phandle);
            Section *s = section_open(ctx, p->name, handle);
            if (s) {
                DPRINTF("%08x: open section %x:%s file:%s\n",
                    (int)ctx, handle, p->name, s->filename);
            } else {
                DPRINTF("%08x: internal error: no open section %s\n",
                    (int)ctx, p->name);
            }
        } else {
            g_free(p->name);
        }
        break;
    }
    case 0x9a: // NtQueryInformationProcess
    {
        /* Current process only */
        address_t *addr = sc->params;
        if (!retval && addr) {
            uint32_t pid = vmi_read_dword(cpu, *addr + 16);
            Process *p = process_get(vmi_get_context(cpu));
            p->pid = pid;
        }
    }
    case 0xb7:
        break;
    case 0x10b: // NtUnmapViewOfSection
        if (!retval) {
            address_t *base = sc->params;
            DPRINTF("%08x: unmapping section base=%x\n",
                (int)ctx, (int)*base);
            mapping_delete(ctx, *base);
        }
        break;
    case 0x112:
        break;
    }
    g_free(sc->params);
    sc->params = NULL;
}
