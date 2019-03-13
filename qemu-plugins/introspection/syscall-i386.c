#include <glib.h>
#include "syscalls.h"
#include "plugins.h"
#include "regnum.h"

typedef struct FileParams {
    address_t phandle;
    wchar_t *name;
} FileParams;

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
    case 0x25:
        qemulib_log("create file: \n");
        break;
    case 0x44:
        qemulib_log("duplicate object: \n");
        break;
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

        wchar_t *str = vmi_strdupw(cpu, buf, len);
        FileParams *p = g_new(FileParams, 1);
        p->phandle = phandle;
        p->name = str;
        return p;
    }
    case 0xb7:
        qemulib_log("read file: \n");
        break;
    case 0x112:
        qemulib_log("write file: \n");
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
    case 0x25:
        qemulib_log("create file: \n");
        break;
    case 0x44:
        qemulib_log("duplicate object: \n");
        break;
    case 0x74: // NtOpenFile
    {
        FileParams *p = sc->params;
        if (!retval) {
            handle_t handle = vmi_read_dword(cpu, p->phandle);
            qemulib_log("open file: %x = %ls\n", (int)handle, p->name);
            file_open(vmi_get_context(cpu), p->name, handle);
            /* Don't free p->name */
        } else {
            g_free(p->name);
        }
        break;
    }
    case 0xb7:
        qemulib_log("read file: \n");
        break;
    case 0x112:
        qemulib_log("write file: \n");
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
            qemulib_log("%llx: sysenter %x\n", vmi_get_context(cpu), reg);
        }
    } else if (code == 0x35) {
        uint32_t ecx = vmi_get_register(cpu, I386_ECX_REGNUM);
        SCData *sc = sc_find(vmi_get_context(cpu), ecx);
        if (sc) {
            qemulib_log("%llx: sysexit %x\n", vmi_get_context(cpu), sc->num);
            syscall_exit_winxp(sc, pc, cpu);
            sc_erase(vmi_get_context(cpu), ecx);
        }
    }
}
