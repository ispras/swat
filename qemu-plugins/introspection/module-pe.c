#include <stdint.h>
#include "plugins.h"
#include "module.h"

#define DEBUG
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

void parse_header_pe()
{
    //address_t base = 
}
