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

#define ELF_DATA ELFDATA2LSB

static bool elf_check_ident(Elf32_Ehdr *ehdr)
{
    DPRINTF("ELF ident %x %x %x %x %x %x %x\n",
        ehdr->e_ident[EI_MAG0], ehdr->e_ident[EI_MAG1],
        ehdr->e_ident[EI_MAG2], ehdr->e_ident[EI_MAG3],
        ehdr->e_ident[EI_CLASS], ehdr->e_ident[EI_DATA],
        ehdr->e_ident[EI_VERSION]);
    return (ehdr->e_ident[EI_MAG0] == ELFMAG0
            && ehdr->e_ident[EI_MAG1] == ELFMAG1
            && ehdr->e_ident[EI_MAG2] == ELFMAG2
            && ehdr->e_ident[EI_MAG3] == ELFMAG3
            && ehdr->e_ident[EI_DATA] == ELF_DATA
            && ehdr->e_ident[EI_VERSION] == EV_CURRENT);
}

/* TODO: endianness */
#define LOAD(var, offs) do { \
    if (qemulib_read_memory(cpu, image_base + (offs), \
        (uint8_t*)&(var), sizeof(var))) { \
        return false; \
    } } while(0)

#define LOAD32(var, offs) do { \
        uint32_t v;            \
        LOAD(v, offs);         \
        var = v;               \
    } while (0)

bool parse_header_elf(cpu_t cpu, Mapping *m)
{
    address_t image_base = m->base;
    address_t size = m->size;
    uint64_t i;
    
    // The header beginning is the same for 32 and 64
    Elf32_Ehdr ehdr;
    LOAD(ehdr, 0);

    // First of all, some simple consistency checks
    if (!elf_check_ident(&ehdr)) {
        return true;
    }
    // skip other checks

    bool is64 = ehdr.e_ident[EI_CLASS] == ELFCLASS64;

    // read program header
    uint16_t phnum, phentsize;
    uint64_t phoff;
    if (is64) {
        LOAD(phnum, offsetof(Elf64_Ehdr, e_phnum));
        LOAD(phentsize, offsetof(Elf64_Ehdr, e_phentsize));
        LOAD(phoff, offsetof(Elf64_Ehdr, e_phoff));
    } else {
        LOAD(phnum, offsetof(Elf32_Ehdr, e_phnum));
        LOAD(phentsize, offsetof(Elf32_Ehdr, e_phentsize));
        LOAD32(phoff, offsetof(Elf32_Ehdr, e_phoff));
    }
    DPRINTF("program header off=0x%llx entsize=0x%x num=0x%x\n", phoff, phentsize, phnum);

    uint64_t dynoff = -1ULL, dynvaddr = -1ULL, dynsz = 1;
    for (i = 0 ; i < phnum ; ++i) {
        uint64_t off = phoff + i * phentsize;
        uint32_t type;
        uint64_t offset, vaddr, filesz, memsz;
        if (is64) {
            LOAD(type, off + offsetof(Elf64_Phdr, p_type));
            LOAD(offset, off + offsetof(Elf64_Phdr, p_offset));
            LOAD(vaddr, off + offsetof(Elf64_Phdr, p_vaddr));
            LOAD(filesz, off + offsetof(Elf64_Phdr, p_filesz));
            LOAD(memsz, off + offsetof(Elf64_Phdr, p_memsz));
        } else {
            LOAD(type, off + offsetof(Elf32_Phdr, p_type));
            LOAD32(offset, off + offsetof(Elf32_Phdr, p_offset));
            LOAD32(vaddr, off + offsetof(Elf32_Phdr, p_vaddr));
            LOAD32(filesz, off + offsetof(Elf32_Phdr, p_filesz));
            LOAD32(memsz, off + offsetof(Elf32_Phdr, p_memsz));
        }
        if (type == PT_DYNAMIC) {
            dynoff = offset;
            dynvaddr = vaddr;
            dynsz = filesz;
            if (!dynsz) {
                dynsz = memsz;
            }
        }
        DPRINTF("\tentry type=0x%x off=0x%llx vaddr=0x%llx filesz=0x%llx memsz=0x%llx\n",
            type, offset, vaddr, filesz, memsz);
    }

    if (dynoff == -1ULL || dynvaddr == -1ULL) {
        DPRINTF("unknown format\n");
        return true;
    }

    // read dynamic section
    DPRINTF("dynamic section entries\n");
    uint64_t strtab = 0, symtab = 0, strsz = 0, syment = 0;
    for (i = 0 ; i < dynsz ;
        i += is64 ? sizeof(Elf64_Dyn) : sizeof(Elf32_Dyn)) {
        uint64_t tag, val;
        if (is64) {
            LOAD(tag, dynvaddr + i + offsetof(Elf64_Dyn, d_tag));
            LOAD(val, dynvaddr + i + offsetof(Elf64_Dyn, d_un));
        } else {
            LOAD32(tag, dynvaddr + i + offsetof(Elf32_Dyn, d_tag));
            LOAD32(val, dynvaddr + i + offsetof(Elf32_Dyn, d_un));
        }
        DPRINTF("\tentry tag=0x%llx val=0x%llx\n", tag, val);

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
        DPRINTF("unknown format %llx %llx %llx %llx\n",
            strtab, symtab, strsz, syment);
        return true;
    }
    // read symbol table
    DPRINTF("symbol table entries from 0x%llx to 0x%llx\n",
        symtab, strtab);
    uint64_t s;
    for (s = symtab ; s < strtab ; s += syment) {
        uint32_t name;
        uint64_t value;
        uint8_t info;
        if (is64) {
            LOAD(name, s + offsetof(Elf64_Sym, st_name));
            LOAD(value, s + offsetof(Elf64_Sym, st_value));
            LOAD(info, s + offsetof(Elf64_Sym, st_info));
        } else {
            LOAD(name, s + offsetof(Elf32_Sym, st_name));
            LOAD32(value, s + offsetof(Elf32_Sym, st_value));
            LOAD(info, s + offsetof(Elf32_Sym, st_info));
        }
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

            DPRINTF("Function: %llx:%s\n", addr + image_base, str);
            char *fullname = g_strjoin(":", m->filename, str, NULL);
            function_add(vmi_get_context(cpu), addr + image_base, fullname);
            g_free(fullname);
        }
    }

    DPRINTF("parsing is finished\n");
    return true;
}
