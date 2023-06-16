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
#include <kbelf/reloc.h>

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
#define store(type, ptr, in) do { \
		for (int i = 0; i < sizeof(type); i++) { \
			ptr[i] = in >> (8 * i); \
		} \
	} while(0)




// Obtain the value of an implicit addend.
kbelf_addr kbelfp_reloc_get_addend(kbelf_file file, kbelf_inst inst, uint32_t type, const uint8_t *ptr) {
	// TODO.
	return 0;
}



#define A addend
#define S sym
#define B (inst->segments[0].vaddr_real - inst->segments[0].vaddr_req)

// Apply a relocation.
bool kbelfp_reloc_apply(kbelf_file file, kbelf_inst inst, uint32_t type, kbelf_addr sym, kbelf_addr addend, uint8_t *ptr) {
	switch ((riscv_reloc_t) type) {
		case ABS32:
			store(uint32_t, ptr, S + A);
			return true;
			
		case ABS64:
			store(uint64_t, ptr, S + A);
			return true;
			
		case RELATIVE:
			store(kbelf_addr, ptr, B + A);
			return true;
			
		case JUMP_SLOT:
			store(kbelf_addr, ptr, S);
			return true;
			
			// TODO: TLS_* relocations.
			
		default:
			return false;
	}
}
