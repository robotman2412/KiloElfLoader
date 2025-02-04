/*
    MIT License

    Copyright (c) 2025 Julian Scheffers

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

#define KBELF_REVEAL_PRIVATE
#include <kbelf.h>
#include <kbelf/port.h>



/* ==== Verification ==== */

// Perform target-specific verification of `kbelf_file`.
bool kbelfp_file_verify(kbelf_file file) {
    return true;
}



/* ==== Relocation ==== */

/*
A			Addend field in the relocation entry associated with the symbol
B			Base address of a shared object loaded into memory
G			Offset of the symbol into the GOT (Global Offset Table)
GOT			Address of the GOT (Global Offset Table)
P			Position of the relocation
S			Value of the symbol in the symbol table
V			Value at the position of the relocation
GP			Value of __global_pointer$ symbol
TLSMODULE	TLS module index for the object containing the symbol
TLSOFFSET	TLS static block offset (relative to tp) for the object containing the symbol
*/

// AMD64 relocation enum.
typedef enum {
    R_AMD64_NONE      = 0,
    R_AMD64_64        = 1,
    R_AMD64_PC32      = 2,
    R_AMD64_GOT32     = 3,
    R_AMD64_PLT32     = 4,
    R_AMD64_COPY      = 5,
    R_AMD64_GLOB_DAT  = 6,
    R_AMD64_JUMP_SLOT = 7,
    R_AMD64_RELATIVE  = 8,
    R_AMD64_GOTPCREL  = 9,
    R_AMD64_32        = 10,
    R_AMD64_32S       = 11,
    R_AMD64_16        = 12,
    R_AMD64_PC16      = 13,
    R_AMD64_8         = 14,
    R_AMD64_PC8       = 15,
    R_AMD64_PC64      = 24,
    R_AMD64_GOTOFF64  = 25,
    R_AMD64_GOTPC32   = 26,
    R_AMD64_SIZE32    = 32,
    R_AMD64_SIZE64    = 33,
} riscv_reloc_t;

// STORE TEMPLATE.
#define store(type, in)                                                                                                \
    do {                                                                                                               \
        type tmp = (in);                                                                                               \
        kbelfx_copy_to_user(inst, laddr, &tmp, sizeof(type));                                                          \
    } while (0)

// Obtain the value of an implicit addend.
kbelf_addr kbelfp_reloc_get_addend(kbelf_file file, kbelf_inst inst, uint32_t type, uint8_t const *ptr) {
    (void)file;
    (void)inst;
    (void)type;
    (void)ptr;
    // TODO.
    return 0;
}

#define A addend
#define B (inst->segments[0].vaddr_real - inst->segments[0].vaddr_req)
#define G
#define GOT
#define L
#define P (kbelf_inst_laddr_to_vaddr(inst, laddr))
#define S sym
#define Z

// Apply a relocation.
bool kbelfp_reloc_apply(
    kbelf_file file, kbelf_inst inst, uint32_t type, kbelf_addr sym, kbelf_addr addend, kbelf_laddr laddr
) {
    (void)file;
    // TODO: TLS relocations.
    switch ((riscv_reloc_t)type) {
        case R_AMD64_NONE: return true;

        case R_AMD64_64: store(uint64_t, S + A); return true;

        case R_AMD64_PC32: store(uint32_t, S + A - P); return true;

        case R_AMD64_COPY: return true;

        case R_AMD64_GLOB_DAT: store(uint64_t, S); return true;

        case R_AMD64_JUMP_SLOT: store(uint64_t, S); return true;

        case R_AMD64_RELATIVE: store(uint64_t, B + A); return true;

        case R_AMD64_32: store(uint32_t, S + A); return true;

        case R_AMD64_32S: store(uint32_t, S + A); return true;

        case R_AMD64_16: store(uint16_t, S + A); return true;

        case R_AMD64_8: store(uint8_t, S + A); return true;

        default: return false;
    }
}
