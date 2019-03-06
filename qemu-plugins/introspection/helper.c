#include <glib.h>
#include "introspection.h"
#include "plugins.h"

uint64_t vmi_get_register(cpu_t cpu, int reg)
{
    uint64_t retval = 0;
    qemulib_read_register(cpu, (uint8_t*)&retval, reg);
    return retval;
}

uint16_t vmi_read_word(cpu_t cpu, address_t addr)
{
    uint16_t res = 0;
    qemulib_read_memory(cpu, addr, (uint8_t*)&res, sizeof(res));
    // TODO: swap bytes
    return res;
}

uint32_t vmi_read_dword(cpu_t cpu, address_t addr)
{
    uint32_t res = 0;
    qemulib_read_memory(cpu, addr, (uint8_t*)&res, sizeof(res));
    // TODO: swap bytes
    return res;
}

char *vmi_strdup(cpu_t cpu, address_t addr, address_t maxlen)
{
    if (!addr) {
        return NULL;
    }
    uint8_t c;
    uint64_t len = 0;
    do {
        qemulib_read_memory(cpu, addr + len, &c, 1);
        ++len;
    } while (c && (!maxlen || len < maxlen));
    char *str = g_malloc(len + 1);
    qemulib_read_memory(cpu, addr, str, len);
    str[len] = 0;
    return str;
}

char *vmi_strdupw(cpu_t cpu, address_t addr, address_t maxlen)
{
    if (!addr) {
        return NULL;
    }
    // TODO: swap bytes
    uint16_t c;
    uint64_t len = 0;
    uint64_t bytes = 0;
    do {
        qemulib_read_memory(cpu, addr + len * 2, (uint8_t*)&c, 2);
        ++len;
        bytes += g_unichar_to_utf8(c, NULL);
    } while (c && (!maxlen || len < maxlen));
    char *str = g_malloc(bytes + 1);
    len = 0;
    bytes = 0;
    do {
        qemulib_read_memory(cpu, addr + len * 2, (uint8_t*)&c, 2);
        bytes += g_unichar_to_utf8(c, str + bytes);
        ++len;
    } while (c && (!maxlen || len < maxlen));
    str[bytes] = 0;
    return str;
}
