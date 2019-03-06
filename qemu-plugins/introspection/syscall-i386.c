#include <glib.h>
#include "syscalls.h"
#include "plugins.h"
#include "regnum.h"

typedef struct FileParams {
    address_t phandle;
    char *name;
} FileParams;

typedef struct DuplicateParams {
    File *f;
    address_t phandle;
    // TODO: process handle
} DuplicateParams;

static void *syscall_enter_winxp(uint32_t sc, address_t pc, cpu_t cpu)
{
    void *params = NULL;
    address_t data = vmi_get_register(cpu, I386_EDX_REGNUM) + 8;
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
    case 0x44: // NtDuplicateObject
    {
        // TODO: can't identify the process by handle
        uint32_t p1, p2;
        p1 = vmi_read_dword(cpu, data);
        p2 = vmi_read_dword(cpu, data + 8);
        // current process
        if (p1 == 0xffffffff && p2 == p1) {
            address_t phandle = vmi_read_dword(cpu, data + 4);
            handle_t handle = vmi_read_dword(cpu, phandle);
            address_t out = vmi_read_dword(cpu, data + 12);
            File *f = file_find(vmi_get_context(cpu), handle);
            if (f) {
                DuplicateParams *p = g_new(DuplicateParams, 1);
                p->f = f;
                p->phandle = out;
                return p;
            }
        } else {
            qemulib_log("unsupported duplicate object\n");
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
            qemulib_log("unsupported query information for handle %x class %d\n",
                (int)handle, info);
        }
    }
    case 0xb7: // NtReadFile
        break;
    case 0x112: // NtWriteFile
        break;
    }
    return params;
}

static void syscall_exit_winxp(SCData *sc, address_t pc, cpu_t cpu)
{
    uint32_t retval = vmi_get_register(cpu, I386_EAX_REGNUM);
    switch (sc->num)
    {
    case 0x19: // NtClose
    {
        if (!retval) {
            handle_t *h = sc->params;
            qemulib_log("close handle: %x\n", (int)*h);
            file_close(vmi_get_context(cpu), *h);
        }
        break;
    }
    case 0x44: // NtDuplicateObject
    {
        DuplicateParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_dword(cpu, p->phandle);
            qemulib_log("duplicate %x = %x = %ls\n",
                (int)handle, (int)p->f->handle, p->f->filename);
            file_open(vmi_get_context(cpu), g_strdup(p->f->filename), handle);
        }
        break;
    }
    case 0x25: // NtCreateFile
        qemulib_log("create ");
        /* Fallthrough */
    case 0x74: // NtOpenFile
    {
        FileParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_dword(cpu, p->phandle);
            qemulib_log("open file: %x = %s\n", (int)handle, p->name);
            file_open(vmi_get_context(cpu), p->name, handle);
            /* Don't free p->name */
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
    case 0x112:
        break;
    }
    g_free(sc->params);
    sc->params = NULL;
}

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
    }
    return false;
}

bool syscall_i386(address_t pc, cpu_t cpu)
{
    uint8_t code = 0;
    qemulib_read_memory(cpu, pc + 1, &code, 1);
    /* log system calls */
    if (code == 0x34) {
        /* Read EAX which contains system call ID */
        uint32_t reg = vmi_get_register(cpu, I386_EAX_REGNUM);
        void *params = syscall_enter_winxp(reg, pc, cpu);
        if (params) {
            sc_insert(vmi_get_context(cpu),
                vmi_get_stack_pointer(cpu), reg, params);
            // qemulib_log("%llx: sysenter %x\n", vmi_get_context(cpu), reg);
        }
    } else if (code == 0x35) {
        uint32_t ecx = vmi_get_register(cpu, I386_ECX_REGNUM);
        SCData *sc = sc_find(vmi_get_context(cpu), ecx);
        if (sc) {
            // qemulib_log("%llx: sysexit %x\n", vmi_get_context(cpu), sc->num);
            syscall_exit_winxp(sc, pc, cpu);
            sc_erase(vmi_get_context(cpu), ecx);
        }
    }
}
