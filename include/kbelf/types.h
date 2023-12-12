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
#include <kbelf/elfspec.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef KBELF_REVEAL_PRIVATE
struct struct_kbelf_file;
struct struct_kbelf_inst;
struct struct_kbelf_dyn;

// Context used to read, write, load and relocate ELF files.
typedef struct struct_kbelf_file  *kbelf_file;
// Loaded instance of an ELF file.
typedef struct struct_kbelf_inst  *kbelf_inst;
// Context used to perform relocation.
typedef struct struct_kbelf_reloc *kbelf_reloc;
// Context used to load and interpret dynamic executables.
typedef struct struct_kbelf_dyn   *kbelf_dyn;
#else
// Context used to read, write, load and relocate ELF files.
typedef void *kbelf_file;
// Loaded instance of an ELF file.
typedef void *kbelf_inst;
// Context used to perform relocation.
typedef void *kbelf_reloc;
// Context used to load and interpret dynamic executables.
typedef void *kbelf_dyn;
#endif

// Loaded segment offset information.
typedef struct {
    // Cookie value from segment memory allocator.
    // Not used or modified by KBELF.
    void *alloc_cookie;
    // Identifier value as specified when program loading was initiated.
    int   pid;

    // Loaded segment memory address.
    // Used to read and write segment data.
    // Equal to `paddr` unless KBELF is cross-compiled.
    kbelf_laddr laddr;
    // Loaded segment physical address.
    // Used to read and set up memory mapping.
    kbelf_addr  paddr;
    // Loaded segment virtual address.
    // Used to perform segment relocations.
    kbelf_addr  vaddr_real;
    // Virtual address as requested by the ELF file.
    kbelf_addr  vaddr_req;
    // Size in memory.
    kbelf_addr  size;
    // Offset in file.
    long        file_off;
    // Size in file.
    long        file_size;

    // Loaded segment read permission.
    bool r;
    // Loaded segment write permission.
    bool w;
    // Loaded segment exec permission.
    bool x;
} kbelf_segment;

// Symbol definition for a built-in library.
typedef struct {
    // Symbol name.
    char const *name;
    // Symbol physical address.
    size_t      paddr;
    // Symbol virtual address.
    size_t      vaddr;
    // Reserved.
    size_t      _reserved;
} kbelf_builtin_sym;

// Definition for a built-in library.
typedef struct {
    // Library path.
    char const              *path;
    // Number of symbols.
    size_t                   symbols_len;
    // Array of symbols.
    kbelf_builtin_sym const *symbols;
    // Reserved.
    size_t                   _reserved;
} kbelf_builtin_lib;

#ifdef KBELF_REVEAL_PRIVATE
// Context used to read, write, load and relocate ELF files.
struct struct_kbelf_file {
    // File descriptor used for loading.
    // Not closed by KBELF.
    void       *fd;
    // Copy of the path of the ELF file.
    char       *path;
    // Sub-string of the path that is the name.
    char const *name;

    // A copy of the header information.
    kbelf_header header;
    // Length of the string table.
    size_t       strtab_len;
    // A copy of the string table.
    char        *strtab;
    // Length of the section name table.
    size_t       shstr_len;
    // A copy of the section name table.
    char        *shstr;
};

// Loaded instance of an ELF file.
struct struct_kbelf_inst {
    // Copy of the path of the source ELF file.
    char       *path;
    // Sub-string of the path that is the name.
    char const *name;
    // Identifier value as specified when program loading was initiated.
    int         pid;

    // Number of loaded segments.
    size_t         segments_len;
    // Information about loaded segments.
    kbelf_segment *segments;

    // Entrypoint address, if any.
    kbelf_addr entry;
    // Virtual address of initialisation function, if any.
    kbelf_addr init_func;
    // Virtual address of finilasation function, if any.
    kbelf_addr fini_func;
    // Number of preinit_array entries.
    size_t     preinit_array_len;
    // Virtual address of preinit_array, if any.
    kbelf_addr preinit_array;
    // Number of init_array entries.
    size_t     init_array_len;
    // Virtual address of init_array, if any.
    kbelf_addr init_array;
    // Number of init_array entries.
    size_t     fini_array_len;
    // Virtual address of init_array, if any.
    kbelf_addr fini_array;

    // Length of the dynamic table.
    size_t                dynamic_len;
    // Load address of the dynamic table.
    kbelf_dynentry const *dynamic;
    // Length of the dynamic string table.
    size_t                dynstr_len;
    // Load address of the dynamic string table.
    char const           *dynstr;
    // Number of dynamic symbols.
    size_t                dynsym_len;
    // Load address of dynamic symbol table.
    kbelf_symentry const *dynsym;
};

// Context used to perform relocation.
struct struct_kbelf_reloc {
    // Number of loaded ELF files.
    size_t                    libs_len;
    // Source ELF files.
    kbelf_file               *libs_file;
    // Loaded instances.
    kbelf_inst               *libs_inst;
    // Number of built-in libraries.
    size_t                    builtins_len;
    // Built-in libraries.
    kbelf_builtin_lib const **builtins;
};

// Context used to load and interpret dynamic executables.
struct struct_kbelf_dyn {
    // Original executable file.
    kbelf_file exec_file;
    // Loaded executable file.
    kbelf_inst exec_inst;
    // Identifier value as specified when program loading was initiated.
    int        pid;

    // Number of loaded library files.
    size_t      libs_len;
    // Source library files.
    kbelf_file *libs_file;
    // Loaded libraries.
    kbelf_inst *libs_inst;

    // Number of built-in libraries.
    size_t                    builtins_len;
    // Built-in libraries.
    kbelf_builtin_lib const **builtins;

    // Entrypoint virtual address.
    kbelf_addr entrypoint;

    // Total number of initialisation functions.
    size_t init_len;
    // Total number of finalisation functions.
    size_t fini_len;

    // Number of entries in the initialisation order.
    size_t  init_order_len;
    // Initialisation order of the libraries by index.
    size_t *init_order;
};
#endif



#ifdef KBELF_REVEAL_PRIVATE

#ifndef KBELF_FMT_CSTR
#define KBELF_FMT_CSTR "%s"
#endif

#ifndef KBELF_FMT_DEC
#define KBELF_FMT_DEC "%d"
#endif

#ifndef KBELF_FMT_SIZE
#define KBELF_FMT_SIZE "%zu"
#endif

#ifndef KBELF_FMT_BYTE
#define KBELF_FMT_BYTE "%02x"
#endif

#ifndef KBELF_LOGGER
#include <stdio.h>
#define KBELF_LOGGER(lv, fmt, ...) printf(lv fmt "\033[0m\n" __VA_OPT__(, ) __VA_ARGS__)
#endif

#ifndef KBELF_LOG_LEVEL_D
#define KBELF_LOG_LEVEL_D "\033[34m"
#endif

#ifndef KBELF_LOG_LEVEL_I
#define KBELF_LOG_LEVEL_I "\033[32m"
#endif

#ifndef KBELF_LOG_LEVEL_W
#define KBELF_LOG_LEVEL_W "\033[33m"
#endif

#ifndef KBELF_LOG_LEVEL_E
#define KBELF_LOG_LEVEL_E "\033[31m"
#endif

#define KBELF_FILENAME (kbelfq_strrchr(__FILE__, '/') ? kbelfq_strrchr(__FILE__, '/') + 1 : __FILE__)

#ifdef KBELF_DEBUG
#define KBELF_LOGD(fmt, ...)                                                                                           \
    {                                                                                                                  \
        KBELF_LOGGER(                                                                                                  \
            KBELF_LOG_LEVEL_D,                                                                                         \
            "KBELF: " KBELF_FMT_CSTR ":" KBELF_FMT_DEC ": " fmt,                                                       \
            KBELF_FILENAME,                                                                                            \
            __LINE__ __VA_OPT__(, ) __VA_ARGS__                                                                        \
        );                                                                                                             \
    }
#else
#define KBELF_LOGD(fmt, ...)                                                                                           \
    {}
#endif

#define KBELF_LOGI(fmt, ...)                                                                                           \
    {                                                                                                                  \
        KBELF_LOGGER(                                                                                                  \
            KBELF_LOG_LEVEL_I,                                                                                         \
            "KBELF: " KBELF_FMT_CSTR ":" KBELF_FMT_DEC ": " fmt,                                                       \
            KBELF_FILENAME,                                                                                            \
            __LINE__ __VA_OPT__(, ) __VA_ARGS__                                                                        \
        );                                                                                                             \
    }

#define KBELF_LOGW(fmt, ...)                                                                                           \
    {                                                                                                                  \
        KBELF_LOGGER(                                                                                                  \
            KBELF_LOG_LEVEL_W,                                                                                         \
            "KBELF: " KBELF_FMT_CSTR ":" KBELF_FMT_DEC ": " fmt,                                                       \
            KBELF_FILENAME,                                                                                            \
            __LINE__ __VA_OPT__(, ) __VA_ARGS__                                                                        \
        );                                                                                                             \
    }

#define KBELF_LOGE(fmt, ...)                                                                                           \
    {                                                                                                                  \
        KBELF_LOGGER(                                                                                                  \
            KBELF_LOG_LEVEL_E,                                                                                         \
            "KBELF: " KBELF_FMT_CSTR ":" KBELF_FMT_DEC ": " fmt,                                                       \
            KBELF_FILENAME,                                                                                            \
            __LINE__ __VA_OPT__(, ) __VA_ARGS__                                                                        \
        );                                                                                                             \
    }

#define KBELF_ERROR(jumplabel, reason, ...)                                                                            \
    { KBELF_LOGE(reason __VA_OPT__(, ) __VA_ARGS__) goto jumplabel; }
#endif



#ifdef __cplusplus
} // extern "C"
#endif
