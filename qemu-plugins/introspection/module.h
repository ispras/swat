#ifndef MODULE_H
#define MODULE_H

#include "introspection.h"

bool parse_header_pe(cpu_t cpu, Mapping *m);
bool parse_header_elf(cpu_t cpu, Mapping *m);

#endif // MODULE_H
