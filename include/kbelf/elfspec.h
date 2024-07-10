/*
    MIT License

    Copyright (c) 2023 Julian Scheffers

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

#include <kbelf/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if KBELF_IS_ELF64
#define KBELF_CLASS 2
#else
#define KBELF_CLASS 1
#endif
#define KBELF_ENDIANNESS 1



// Load address type.
typedef size_t kbelf_laddr;
#if KBELF_IS_ELF64
// ELF address type.
typedef uint64_t kbelf_addr;
// ELF addrdiff type.
typedef int64_t  kbelf_addrdiff;
#else
// ELF address type.
typedef uint32_t kbelf_addr;
// ELF addrdiff type.
typedef int32_t  kbelf_addrdiff;
#endif

// First four bytes of an ELF file.
static char const     kbelf_magic[4] = {0x7f, 'E', 'L', 'F'};
// Machine type to check against.
extern uint16_t const kbelf_machine_type;


// ELF file type.
typedef enum {
    ET_NONE = 0x00,
    ET_REL  = 0x01,
    ET_EXEC = 0x02,
    ET_DYN  = 0x03,
} kbelf_et;

// Section header type.
typedef enum {
    SHT_PROGBITS = 0x01,
    SHT_SYMTAB   = 0x02,
    SHT_STRTAB   = 0x03,
    SHT_RELA     = 0x04,
    SHT_HASH     = 0x05,
    SHT_DYNAMIC  = 0x06,
    SHT_NOTE     = 0x07,
    SHT_NOBITS   = 0x08,
    SHT_REL      = 0x09,
    SHT_SHLIB    = 0x0a,
    SHT_DYNSYM   = 0x0b,
} kbelf_sht;

// Special section index: Absolute.
#define SHN_ABS    0xfff1
// Special section index: Common.
#define SHN_COMMON 0xfff2
// Special section index: Undefined.
#define SHN_UNDEF  0x0000

// Program header type.
typedef enum {
    PT_UNUSED  = 0x00,
    PT_LOAD    = 0x01,
    PT_DYNAMIC = 0x02,
    PT_INTERP  = 0x03,
    PT_NOTE    = 0x04,
    PT_SHLIB   = 0x05,
    PT_PHDR    = 0x06,
    PT_TLS     = 0x07,
} kbelf_pt;

// Program header flags: Execute.
#define PF_X 0x01
// Program header flags: Write.
#define PF_W 0x02
// Program header flags: Read.
#define PF_R 0x04

// Symbol type.
typedef enum {
    STT_NOTYPE  = 0x00,
    STT_OBJECT  = 0x01,
    STT_FUNC    = 0x02,
    STT_SECTION = 0x03,
    STT_FILE    = 0x04,
} kbelf_stt;

// Symbol binding.
typedef enum {
    STB_LOCAL  = 0x00,
    STB_GLOBAL = 0x01,
    STB_WEAK   = 0x02,
} kbelf_stb;

// Dynamic entry tag.
typedef enum {
    DT_NULL            = 0x00,
    DT_NEEDED          = 0x01,
    DT_PLTRELSZ        = 0x02,
    DT_PLTGOT          = 0x03,
    DT_HASH            = 0x04,
    DT_STRTAB          = 0x05,
    DT_SYMTAB          = 0x06,
    DT_RELA            = 0x07,
    DT_RELASZ          = 0x08,
    DT_RELAENT         = 0x09,
    DT_STRSZ           = 0x0a,
    DT_SYMENT          = 0x0b,
    DT_INIT            = 0x0c,
    DT_FINI            = 0x0d,
    DT_SONAME          = 0x0e,
    DT_RPATH           = 0x0f,
    DT_SYMBOLIC        = 0x10,
    DT_REL             = 0x11,
    DT_RELSZ           = 0x12,
    DT_RELENT          = 0x13,
    DT_PLTREL          = 0x14,
    DT_DEBUG           = 0x15,
    DT_TEXTREL         = 0x16,
    DT_JMPREL          = 0x17,
    DT_BIND_NOW        = 0x18,
    DT_INIT_ARRAY      = 0x19,
    DT_FINI_ARRAY      = 0x1a,
    DT_INIT_ARRAYSZ    = 0x1b,
    DT_FINI_ARRAYSZ    = 0x1c,
    DT_RUNPATH         = 0x1d,
    DT_FLAGS           = 0x1e,
    DT_ENCODING        = 0x1f,
    DT_PREINIT_ARRAY   = 0x20,
    DT_PREINIT_ARRAYSZ = 0x21,
} kbelf_dt;


// Common (32-bit and 64-bit) ELF file header information.
typedef struct {
    // Magic: 0x7f, 'E', 'L', 'F'.
    char       magic[4];
    // Class: 1 or 2 for 32- or 64-bit respectively.
    uint8_t    word_size;
    // Endianness: 1 or 2 for little or big respectively.
    uint8_t    endianness;
    // ELF file version.
    uint8_t    version;
    // OS/ABI type.
    uint8_t    osabi;
    // More OS/ABI data.
    uint8_t    ident;
    // Padding bytes.
    char       _padding0[7];
    // ELF file type.
    uint16_t   type;
    // Machine type.
    uint16_t   machine;
    // A copy of the version.
    uint32_t   version2;
    // Program entry point.
    kbelf_addr entry;
    // File offset of program header table.
    kbelf_addr ph_offset;
    // File offset of section header table.
    kbelf_addr sh_offset;
    // Target-dependent flags.
    uint32_t   flags;
    // Size of this header, 64 bytes for 64-bit or 52 bytes for 32-bit.
    uint16_t   size;
    // Size of a program header table entry.
    uint16_t   ph_ent_size;
    // Number of entries in the program header table.
    uint16_t   ph_ent_num;
    // Size of a program header table entry.
    uint16_t   sh_ent_size;
    // Number of entries in the program header table.
    uint16_t   sh_ent_num;
    // Index of the section containing the section name table.
    uint16_t   sh_str_index;
} kbelf_header;

// Section header information.
typedef struct {
    // Index in the name table.
    uint32_t   name_index;
    // Type of section.
    uint32_t   type;
    // Flags bitmap.
    kbelf_addr flags;
    // Virtual address for loadable sections.
    kbelf_addr vaddr;
    // Offset in the file image of section data.
    kbelf_addr offset;
    // Size in the file image.
    kbelf_addr file_size;
    // A related section, if any.
    uint32_t   link;
    // Type dependent additional info.
    uint32_t   info;
    // Alignment, must be an integer power of two.
    kbelf_addr alignment;
    // The size, in bytes, of an entry, for sections with fixed-size entries.
    kbelf_addr entry_size;
} kbelf_sectheader;

// Program header information.
typedef struct {
    // Type of the segment.
    uint32_t type;
#if KBELF_IS_ELF64
    // Flags bitfield.
    uint32_t flags;
#endif
    // Offset in the file image.
    kbelf_addr offset;
    // Virtual address of segment.
    kbelf_addr vaddr;
    // Physical address, if any.
    kbelf_addr paddr;
    // Size in the file image in bytes.
    kbelf_addr file_size;
    // Size in memory.
    kbelf_addr mem_size;
#if !KBELF_IS_ELF64
    // Flags bitfield.
    uint32_t flags;
#endif
    // Alignment, must be an integer power of two.
    kbelf_addr alignment;
} kbelf_progheader;

// Symbol table entry.
typedef struct {
    // Index in the name table.
    uint32_t name_index;
#if !KBELF_IS_ELF64
    // Symbol value, if any.
    kbelf_addr value;
    // Symbol size in bytes.
    uint32_t   size;
#endif
    // Type and attributes.
    uint8_t  info;
    // Symbol visibility.
    uint8_t  other;
    // Section index of this symbol. 0 means the symbol is undefined.
    uint16_t section;
#if KBELF_IS_ELF64
    // Symbol value, if any.
    kbelf_addr value;
    // Symbol size in bytes.
    uint64_t   size;
#endif
} kbelf_symentry;

// Get the `bind` value from a symbol entry's `info` field.
#define KBELF_ST_BIND(x)          ((x) >> 4)
// Get the `type` value from a symbol entry's `info` field.
#define KBELF_ST_TYPE(x)          ((x) & 15)
// Combine the `bind` and `type` values into a symbol entry's `info` field.
#define KBELF_ST_INFO(bind, type) (((bind) << 4) | ((type) & 15))

// Dynamic table entry.
typedef struct {
    // Type of info stored in this entry.
    kbelf_addr tag;
    // Pointer to data or value of entry.
    kbelf_addr value;
} kbelf_dynentry;

// Relocation table entry (without addend).
typedef struct {
    // Offset in the subject section.
    kbelf_addr offset;
    // Symbol index to apply to, relocation type.
    kbelf_addr info;
} kbelf_relentry;

// Relocation table entry (with addend).
typedef struct {
    // Offset in the subject section.
    kbelf_addr     offset;
    // Symbol index to apply to, relocation type.
    kbelf_addr     info;
    // Addend.
    kbelf_addrdiff addend;
} kbelf_relaentry;

// Get the `symbol` value from a relocation entry's `info` field.
#define KBELF_R_SYM(x)          ((x) >> 8)
// Get the `type` value from a relocation entry's `info` field.
#define KBELF_R_TYPE(x)         ((x) & 255)
// Conbine the `symbol` and `type` values into a relocation entry's `info` field.
#define KBELF_R_INFO(sym, type) (((sym) << 8) | ((type) & 255))



#ifdef __cplusplus
} // extern "C"
#endif
