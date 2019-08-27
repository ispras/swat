#include <stdint.h>
#include "plugins.h"
#include "module.h"

//#define DEBUG
#ifdef DEBUG
#define DPRINTF qemulib_log
#else
static inline void DPRINTF(const char *fmt, ...) {}
#endif

#define PACKED __attribute__ ((packed))

typedef struct IMAGE_DOS_HEADER {
     uint16_t e_magic;
     uint16_t e_cblp;
     uint16_t e_cp;
     uint16_t e_crlc;
     uint16_t e_cparhdr;
     uint16_t e_minalloc;
     uint16_t e_maxalloc;
     uint16_t e_ss;
     uint16_t e_sp;
     uint16_t e_csum;
     uint16_t e_ip;
     uint16_t e_cs;
     uint16_t e_lfarlc;
     uint16_t e_ovno;
     uint16_t e_res[4];
     uint16_t e_oemid;
     uint16_t e_oeminfo;
     uint16_t e_res2[10];
     uint32_t e_lfanew;
} PACKED IMAGE_DOS_HEADER;

typedef struct PEHeader {
    uint16_t machine;
    uint16_t numberOfSections;
    uint32_t timeDateStamp;
    uint32_t pointerToSymbolTable;
    uint32_t numberOfSymbols;
    uint16_t sizeOfOptionalHeader;
    uint16_t characteristics;
} PACKED PEHeader;

typedef struct IMAGE_DATA_DIRECTORY {
    uint32_t virtualAddress;
    uint32_t size;
} PACKED IMAGE_DATA_DIRECTORY;

typedef struct IMAGE_SECTION_HEADER {
    uint8_t Name[8];
    uint32_t PhysicalAddress;
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} PACKED IMAGE_SECTION_HEADER;

/* TODO: endianness */
#define LOAD(var, offs) do { \
    if (qemulib_read_memory(cpu, image_base + (offs), \
        (uint8_t*)&(var), sizeof(var))) { \
        return false; \
    } } while(0)

bool parse_header_pe(cpu_t cpu, Mapping *m)
{
    address_t image_base = m->base;
    address_t size = m->size;

    // check DOS header
    uint16_t magic;
    LOAD(magic, offsetof(IMAGE_DOS_HEADER, e_magic));
    if (magic != 0x5a4d) {
        DPRINTF("No DOS magic\n");
        return true;
    }
    DPRINTF("DOS magic is ok\n");
    uint32_t base;
    LOAD(base, offsetof(IMAGE_DOS_HEADER, e_lfanew));
    // check PE header
    address_t offs = base;
    uint8_t s1, s2;
    LOAD(s1, offs);
    LOAD(s2, offs + 1);
    if (s1 != 'P' || s2 != 'E') {
        DPRINTF( "No PE magic\n");
        return true;
    }
    DPRINTF( "PE magic is ok\n");

    PEHeader pe_header;

    offs += 4;
    DPRINTF( "header\n");
    // machine
    LOAD(pe_header.machine, offs);
    bool is64 = pe_header.machine == 0x8664;
    offs += 2;
    DPRINTF("machine: 0x%x\n", pe_header.machine);
    // number of sections
    LOAD(pe_header.numberOfSections, offs); // important 
    DPRINTF("number of sections: 0x%x\n", pe_header.numberOfSections);
    offs += 2;
    // time date stamp
#ifdef DEBUG
    LOAD(pe_header.timeDateStamp, offs);
    DPRINTF("time date stamp: 0x%x\n", pe_header.timeDateStamp);
#endif
    offs += 4;
    // pointer to symbol table
#ifdef DEBUG
    LOAD(pe_header.pointerToSymbolTable, offs);
    DPRINTF("pointer to symbol table: 0x%x\n", pe_header.pointerToSymbolTable);
#endif
    offs += 4;
    // number of symbols
#ifdef DEBUG
    LOAD(pe_header.numberOfSymbols, offs);
    DPRINTF("number of symbols: 0x%x\n", pe_header.numberOfSymbols);
#endif
    offs += 4;
    // size of optional header
    LOAD(pe_header.sizeOfOptionalHeader, offs);
    DPRINTF("size of optional header: 0x%x\n", pe_header.sizeOfOptionalHeader);
    offs += 2;
    // characteristics
#ifdef DEBUG
    LOAD(pe_header.characteristics, offs);
    DPRINTF("characteristics: 0x%x\n", pe_header.characteristics);
#endif
    offs += 2;

    uint16_t pe32magic;
    LOAD(pe32magic, offs);
    DPRINTF("pe32 magic: 0x%x\n", pe32magic);
    bool is32plus = pe32magic == 0x20b;
    
    offs += 24;
    uint64_t imageBase;
    if (is32plus) {
        LOAD(imageBase, offs); // 8 bytes in 64-bit mode
        offs += 8;
    } else {
        offs += 4;
        uint32_t imageBase32;
        LOAD(imageBase32, offs); // 8 bytes in 64-bit mode
        offs += 4;
        imageBase = imageBase32;
    }
    DPRINTF("ImageBase: 0x%llx\n", imageBase);
#ifdef DEBUG
    uint32_t sectionAlign;
    LOAD(sectionAlign, offs);
    DPRINTF("SectionAligment: 0x%x\n", sectionAlign);
#endif
    offs += 4;
    uint32_t fileAligment;
    LOAD(fileAligment, offs);
    DPRINTF("FileAligment: 0x%x\n", fileAligment);
    offs += 4;
    
    offs += is32plus ? 72 : 56;

    uint32_t exp_addr = 0;
    int i;
    for (i = 0; i < 16; i++)
    {
        uint32_t var;
        LOAD(var, offs);
        DPRINTF("addr = 0x%x  ", var);
        if (i == 0) {
            LOAD(exp_addr, offs);
        }
        offs += 4;

        LOAD(var, offs);
        DPRINTF("size = 0x%x\n", var);
        offs += 4;
    }
    IMAGE_SECTION_HEADER section_header[pe_header.numberOfSections];
    int k;
    for (k = 0; k < pe_header.numberOfSections; k++)
    {
        int i = 0;
        for (i = 0; i < 8; i++)
        {
            LOAD(section_header[k].Name[i], offs);
            DPRINTF("%c", section_header[k].Name[i]);
            offs++;
        }
        DPRINTF("\n");

        LOAD(section_header[k].VirtualSize, offs);
        DPRINTF("VirtualSize 0x%x\n", section_header[k].VirtualSize);
        offs += 4;

        LOAD(section_header[k].VirtualAddress, offs);
        DPRINTF("VirtualAddress 0x%x\n", section_header[k].VirtualAddress); // section RVA
        offs += 4;

        LOAD(section_header[k].SizeOfRawData, offs);
        DPRINTF("SizeOfRawData 0x%x\n", section_header[k].SizeOfRawData);
        offs += 4;

#ifdef DEBUG
        LOAD(section_header[k].PointerToRawData, offs);
        DPRINTF("PointerToRawData 0x%x\n", section_header[k].PointerToRawData);
#endif
        offs += 4;
#ifdef DEBUG
        LOAD(section_header[k].PointerToRelocations, offs);
        DPRINTF("PointerToRelocations 0x%x\n", section_header[k].PointerToRelocations);
#endif
        offs += 4;
#ifdef DEBUG
        LOAD(section_header[k].PointerToLinenumbers, offs);
        DPRINTF("PointerToLinenumbers 0x%x\n", section_header[k].PointerToLinenumbers);
#endif
        offs += 4;
#ifdef DEBUG
        LOAD(section_header[k].NumberOfRelocations, offs);
        DPRINTF("NumberOfRelocations 0x%x\n", section_header[k].NumberOfRelocations);
#endif
        offs += 2;
#ifdef DEBUG
        LOAD(section_header[k].NumberOfLinenumbers, offs);
        DPRINTF("NumberOfLinenumbers 0x%x\n", section_header[k].NumberOfLinenumbers);
#endif
        offs += 2;
#ifdef DEBUG
        LOAD(section_header[k].Characteristics, offs);
        DPRINTF("Characteristics 0x%x\n", section_header[k].Characteristics);
#endif
        offs += 4;
    }
    
    uint32_t exportRAWaddr = exp_addr;
    DPRINTF("RAW addr = 0x%x\n", exportRAWaddr);
    
    offs = exportRAWaddr;
#ifdef DEBUG
    uint32_t tmp;
    LOAD(tmp, offs);
    DPRINTF("export: characteristics 0x%x\n", tmp);
#endif
    offs += 4;
#ifdef DEBUG
    LOAD(tmp, offs);
    DPRINTF("export: timeDateStamp 0x%x\n", tmp);
#endif
    offs += 4;
#ifdef DEBUG    
    uint16_t tmp2;
    LOAD(tmp2, offs);
    DPRINTF("export: majorVersion 0x%x\n", tmp2);
#endif
    offs += 2;
#ifdef DEBUG
    LOAD(tmp2, offs);
    DPRINTF("export: minorVersion 0x%x\n", tmp2);
#endif
    offs += 2;

    uint32_t nameDLL;
    LOAD(nameDLL, offs);
    DPRINTF("export: name 0x%x\n", nameDLL);
    offs += 4;
    
#ifdef DEBUG
    LOAD(tmp, offs);
    DPRINTF("export: base 0x%x\n", tmp);
#endif
    offs += 4;

    uint32_t numOfFunc;
    LOAD(numOfFunc, offs);
    if (!numOfFunc) {
        DPRINTF("Unknown DLL format\n");
        return true;
    }
    DPRINTF("export: numberOfFunctions 0x%x\n", numOfFunc);
    offs += 4;

    uint32_t numOfNames;
    LOAD(numOfNames, offs);
    DPRINTF("export: numberOfNames 0x%x\n", numOfNames);
    offs += 4;

    uint32_t addrOfFunc;
    LOAD(addrOfFunc, offs);
    DPRINTF("export: addressOfFunctions 0x%x\n", addrOfFunc);
    offs += 4;

    uint32_t addrOfNames;
    LOAD(addrOfNames, offs);
    DPRINTF("export: addressOfNames 0x%x\n", addrOfNames);
    offs += 4;

    uint32_t addrOfNameOrd;
    LOAD(addrOfNameOrd, offs);
    DPRINTF("export: addressOfNameOrdinals 0x%x\n", addrOfNameOrd);
    offs += 4;

    if (!addrOfFunc || !addrOfNameOrd || !addrOfNames) {
        DPRINTF("Unknown PE format\n");
        return true;
    }
    
    /* read names of functions */
    for (i = 0; i < numOfNames; i++) {
        // get function ordinal
        uint16_t ord;
        LOAD(ord, addrOfNameOrd + i * 2);
        // get name address
        uint32_t addr;
        LOAD(addr, addrOfNames + i * 4);
        char name[128];
        int k;
        for (k = 0; k < 127; k++)
        {
            uint8_t symbol;
            LOAD(symbol, addr + k);
            name[k] = symbol;
            if (symbol == 0) {
                break;
            }
        }
        name[k] = 0;
        // get function address
        LOAD(addr, addrOfFunc + ord * 4);

        DPRINTF("Function: %llx:%s\n", addr + image_base, name);
        char *fullname = g_strjoin(":", m->filename, name, NULL);
        function_add(vmi_get_context(cpu), addr + image_base, fullname);
        g_free(fullname);
    }
    DPRINTF("Parsing is finished\n");

    return true;
}
