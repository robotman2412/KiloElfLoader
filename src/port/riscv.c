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

#define KBELF_REVEAL_PRIVATE
#include <kbelf.h>
#include <kbelf/port.h>



/* ==== How to detect RISC-V ==== */
// Define `__riscv` for RISC-V based targets.
// Define `__riscv_a` for atomics extension.
// Define `__riscv_c` for compressed instructions extension.
// Define `__riscv_e` for 16-register base ISA.
// Define `__riscv_i` for integer base ISA.
// Define `__riscv_xlen` as 32, 64 or 128 to detect word size.
// Define `__riscv_float_abi_soft` to detect soft-float.
// Define `__riscv_float_abi_single` to detect single-precision float.
// Define `__riscv_float_abi_double` to detect double-precision float.

/* ==== Verification ==== */

// Header flags: Contains compressed instructions.
#define KBELF_RISCV_FLAG_RVC 0x0001

// Header flags: Floating-point ABI mask.
#define KBELF_RISCV_MASK_FABI        0x0006
// Header flags: Uses soft-float ABI.
#define KBELF_RISCV_FLAG_FABI_SOFT   0x0000
// Header flags: Uses single-precision float ABI.
#define KBELF_RISCV_FLAG_FABI_SINGLE 0x0002
// Header flags: Uses double-precision float ABI.
#define KBELF_RISCV_FLAG_FABI_DOUBLE 0x0004
// Header flags: Uses quadruple-precision float ABI.
#define KBELF_RISCV_FLAG_FABI_QUAD   0x0006

// Header flags: Uses only the RV32E register subset.
#define KBELF_RISCV_FLAG_RVE 0x0008
// Header flags: Requires the RVTSO memory ordering model.
#define KBELF_RISCV_FLAG_TSO 0x0010

#ifndef KBELF_CROSS
// Detect RVC.
#ifdef __riscv_c
#define KBELF_RISCV_HOST_RVC KBELF_RISCV_FLAG_RVC
#else
#define KBELF_RISCV_HOST_RVC 0
#endif

// Detect floating-point ABI.
#ifdef __riscv_float_abi_double
#define KBELF_RISCV_HOST_FABI KBELF_RISCV_FLAG_FABI_DOUBLE
#elif defined(__riscv_float_abi_single)
#define KBELF_RISCV_HOST_FABI KBELF_RISCV_FLAG_FABI_SINGLE
#elif defined(__riscv_float_abi_soft)
#define KBELF_RISCV_HOST_FABI KBELF_RISCV_FLAG_FABI_SOFT
#else
#warning "Unable to detect floating-point ABI: Assuming soft-float."
#define KBELF_RISCV_HOST_FABI KBELF_RISCV_FLAG_FABI_SOFT
#endif

// Detect RV32E.
#ifdef __riscv_e
#define KBELF_RISCV_HOST_RVE KBELF_RISCV_FLAG_RVE
#else
#define KBELF_RISCV_HOST_RVE 0
#endif

// TODO: Detect RVTSO.
#define KBELF_RISCV_HOST_RVTSO 0
#endif

// Perform target-specific verification of `kbelf_file`.
bool kbelfp_file_verify(kbelf_file file) {
    if ((file->header.flags & KBELF_RISCV_FLAG_RVC) && !KBELF_RISCV_HOST_RVC) {
        KBELF_ERROR(abort, "Unsupported machine (RVC requested but not supported)")
    }
    if ((file->header.flags & KBELF_RISCV_MASK_FABI) != KBELF_RISCV_HOST_FABI) {
        char const *fabi[] = {
            "soft-float",
            "single-precision",
            "double-precision",
            "quadruple-precision",
        };
        KBELF_ERROR(
            abort,
            "Unsupported machine (FABI " KBELF_FMT_CSTR " requested; acutal FABI " KBELF_FMT_CSTR ")",
            fabi[(file->header.flags & KBELF_RISCV_MASK_FABI) >> 1],
            fabi[(KBELF_RISCV_HOST_FABI) >> 1]
        )
    }
    if ((file->header.flags & KBELF_RISCV_FLAG_RVE) && !KBELF_RISCV_HOST_RVE) {
        KBELF_ERROR(abort, "Unsupported machine (RVE requested but not supported)")
    }
    if (!(file->header.flags & KBELF_RISCV_FLAG_RVE) && KBELF_RISCV_HOST_RVE) {
        KBELF_ERROR(abort, "Unsupported machine (RVI requested but not supported)")
    }

    return true;

abort:
    return false;
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

// RISC-V relocation enum.
typedef enum {
    ABS32        = 1,
    ABS64        = 2,
    RELATIVE     = 3,
    COPY         = 4,
    JUMP_SLOT    = 5,
    TLS_DTPMOD32 = 6,
    TLS_DTPMOD64 = 7,
    TLS_DTPREL32 = 8,
    TLS_DTPREL64 = 9,
    TLS_TPREL32  = 10,
    TLS_TPREL64  = 11,
    IRELATIVE    = 58,
} riscv_reloc_t;

// STORE TEMPLATE.
#define store(type, ptr, in)                                                                                           \
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
#define S sym
#define B (inst->segments[0].vaddr_real - inst->segments[0].vaddr_req)

// Apply a relocation.
bool kbelfp_reloc_apply(
    kbelf_file file, kbelf_inst inst, uint32_t type, kbelf_addr sym, kbelf_addr addend, kbelf_laddr laddr
) {
    (void)file;
    switch ((riscv_reloc_t)type) {
        case ABS32: store(uint32_t, ptr, S + A); return true;

        case ABS64: store(uint64_t, ptr, S + A); return true;

        case RELATIVE: store(kbelf_addr, ptr, B + A); return true;

        case JUMP_SLOT:
            store(kbelf_addr, ptr, S);
            return true;

            // TODO: TLS_* relocations.

        default: return false;
    }
}
