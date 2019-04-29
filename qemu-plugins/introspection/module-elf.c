#include <stdint.h>
#include "plugins.h"
#include "module.h"
/* TODO: get rid of this dependency */
#include "../../qemu/include/elf.h"

//#define DEBUG
#ifdef DEBUG
#define DPRINTF qemulib_log
#else
static inline void DPRINTF(const char *fmt, ...) {}
#endif

#define PACKED __attribute__ ((packed))

#define elfhdr Elf32_Ehdr
#define elf_sym Elf32_Sym
#define elf_phdr Elf32_Phdr
#define elf_shdr Elf32_Shdr
#define elf_dyn Elf32_Dyn

#define ELF_CLASS ELFCLASS32
#define ELF_DATA ELFDATA2LSB

static bool elf_check_ident(elfhdr *ehdr)
{
    return (ehdr->e_ident[EI_MAG0] == ELFMAG0
            && ehdr->e_ident[EI_MAG1] == ELFMAG1
            && ehdr->e_ident[EI_MAG2] == ELFMAG2
            && ehdr->e_ident[EI_MAG3] == ELFMAG3
            && ehdr->e_ident[EI_CLASS] == ELF_CLASS
            && ehdr->e_ident[EI_DATA] == ELF_DATA
            && ehdr->e_ident[EI_VERSION] == EV_CURRENT);
}

/* TODO: endianness */
#define LOAD(var, offs) do { \
    if (qemulib_read_memory(cpu, image_base + (offs), \
        (uint8_t*)&(var), sizeof(var))) { \
        return false; \
    } } while(0)

bool parse_header_elf(cpu_t cpu, Mapping *m)
{
    address_t image_base = m->base;
    address_t size = m->size;
    int i;
    
    elfhdr ehdr;
    LOAD(ehdr, 0);

    // First of all, some simple consistency checks
    if (!elf_check_ident(&ehdr)) {
        return true;
    }
    // skip other checks

    // read program header
    uint16_t phnum, phentsize;
    uint32_t phoff;
    LOAD(phnum, offsetof(elfhdr, e_phnum));
    LOAD(phentsize, offsetof(elfhdr, e_phentsize));
    LOAD(phoff, offsetof(elfhdr, e_phoff));
    DPRINTF("program header off=0x%x entsize=0x%x num=0x%x\n", phoff, phentsize, phnum);

    uint32_t dynoff = -1, dynvaddr = -1, dynsz = 1;
    for (i = 0 ; i < phnum ; ++i) {
        uint32_t off = phoff + i * phentsize;
        uint32_t type, offset, vaddr, filesz, memsz;
        LOAD(type, off + offsetof(elf_phdr, p_type));
        LOAD(offset, off + offsetof(elf_phdr, p_offset));
        LOAD(vaddr, off + offsetof(elf_phdr, p_vaddr));
        LOAD(filesz, off + offsetof(elf_phdr, p_filesz));
        LOAD(memsz, off + offsetof(elf_phdr, p_memsz));
        if (type == PT_DYNAMIC) {
            dynoff = offset;
            dynvaddr = vaddr;
            dynsz = filesz;
        }
        DPRINTF("\tentry type=0x%x off=0x%x vaddr=0x%x filesz=0x%x memsz=0x%x\n", type, offset, vaddr, filesz, memsz);
    }

    if (dynoff == -1 || dynvaddr == -1) {
        DPRINTF("unknown format\n");
        return true;
    }

    // read dynamic section
    DPRINTF("dynamic section entries\n");
    uint32_t strtab = 0, symtab = 0, strsz = 0, syment = 0;
    for (i = 0 ; i < dynsz ; i += sizeof(elf_dyn)) {
        uint32_t tag, val;
        LOAD(tag, dynvaddr + i + offsetof(elf_dyn, d_tag));
        LOAD(val, dynvaddr + i + offsetof(elf_dyn, d_un));
        DPRINTF("\tentry tag=0x%x val=0x%x\n", tag, val);

        if (tag == DT_STRTAB) {
            // already converted to vaddr by loader
            if (val >= image_base) {
                strtab = val - image_base;
            } else {
                strtab = val;
            }
        } else if (tag == DT_SYMTAB) {
            // already converted to vaddr by loader
            if (val >= image_base) {
                symtab = val - image_base;
            } else {
                symtab = val;
            }
        } else if (tag == DT_STRSZ) {
            strsz = val;
        } else if (tag == DT_SYMENT) {
            syment = val;
        }
    }
    if (!strtab || !symtab || !strsz || !syment) {
        DPRINTF("unknown format\n");
        return true;
    }
    // read symbol table
    DPRINTF("symbol table entries from 0x%x to 0x%x\n", symtab, strtab);
    uint32_t s;
    for (s = symtab ; s < strtab ; s += syment) {
        uint32_t name, value;
        uint8_t info;
        LOAD(name, s + offsetof(elf_sym, st_name));
        LOAD(value, s + offsetof(elf_sym, st_value));
        LOAD(info, s + offsetof(elf_sym, st_info));
        if (ELF_ST_TYPE(info) == STT_FUNC) {
            // found a function
            address_t addr = value;

            char str[128];
            int k;
            address_t pos = strtab + name;
            for (k = 0; k < 127; k++) {
                LOAD(str[k], pos);
                if (!str[k]) {
                    break;
                }
                ++pos;
            }
            str[k] = 0;

            DPRINTF("Function: %x:%s\n", (int)addr + image_base, str);
            function_add(vmi_get_context(cpu), addr + image_base, str);
        }
    }

    DPRINTF("parsing is finished\n");
    return true;
}
