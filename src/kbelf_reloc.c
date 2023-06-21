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

// Create an empty relocation context.
kbelf_reloc kbelf_reloc_create() {
	kbelf_reloc reloc = kbelfx_malloc(sizeof(struct struct_kbelf_reloc));
	if (!reloc) return NULL;
	kbelfq_memset(reloc, 0, sizeof(struct struct_kbelf_reloc));
	return reloc;
}

// Clean up a `kbelf_reloc` context.
void kbelf_reloc_destroy(kbelf_reloc reloc) {
	if (!reloc) return;
	if (reloc->libs_file) kbelfx_free(reloc->libs_file);
	if (reloc->libs_inst) kbelfx_free(reloc->libs_inst);
	kbelfx_free(reloc);
}

// Compute the value of a symbol.
static inline kbelf_addr get_sym_value(kbelf_file file, kbelf_inst inst, kbelf_symentry sym) {
	if (sym.section == SHN_ABS) {
		return sym.value;
	} else {
		return kbelf_inst_getvaddr(inst, sym.value);
	}
}

// Look up a symbol in a relocation context.
static bool find_sym(kbelf_reloc reloc, const char *sym_name, kbelf_addr *out_val) {
	// TODO: Proper handling of "symbolic" (own file first instead of default order) linking.
	bool found = false;
	
	for (size_t x = 0; x < reloc->builtins_len; x++) {
		// Look up builtin library.
		const kbelf_builtin_lib *lib = reloc->builtins[x];
		for (size_t y = 0; y < lib->symbols_len; y++) {
			kbelf_builtin_sym sym = lib->symbols[y];
			if (!kbelfq_streq(sym.name, sym_name)) continue;
			*out_val = sym.vaddr;
			return true;
		}
	}
	
	for (size_t x = 0; x < reloc->libs_len; x++) {
		// Look up a loaded instance.
		kbelf_file file = reloc->libs_file[x];
		kbelf_inst inst = reloc->libs_inst[x];
		for (size_t y = 1; y < inst->dynsym_len; y++) {
			kbelf_symentry sym = inst->dynsym[y];
			// Compare the type.
			if (!sym.section) continue;
			// TODO: Proper handling of local symbols.
			if (KBELF_ST_BIND(sym.info) == STB_LOCAL) continue;
			// Compare the name.
			const char *name = inst->dynstr + sym.name_index;
			if (!kbelfq_streq(name, sym_name)) continue;
			// Eliminate the weak.
			*out_val = get_sym_value(file, inst, sym);
			if (KBELF_ST_BIND(sym.info) != STB_WEAK) return true;
			found = true;
		}
	}
	
	return found;
}

// Perform all relocations from a REL table.
static bool rel_perform(kbelf_reloc reloc, kbelf_file file, kbelf_inst inst, size_t reltab_len, const kbelf_relentry *reltab) {
	// TODO.
	return false;
}

// Perform all relocations from a RELA table.
static bool rela_perform(kbelf_reloc reloc, kbelf_file file, kbelf_inst inst, size_t relatab_len, const kbelf_relaentry *relatab) {
	for (size_t i = 0; i < relatab_len; i++) {
		kbelf_laddr    laddr   = kbelf_inst_getladdr(inst, relatab[i].offset);
		size_t         sym     = KBELF_R_SYM(relatab[i].info);
		uint_fast8_t   type    = KBELF_R_TYPE(relatab[i].info);
		kbelf_addrdiff addend  = relatab[i].addend;
		kbelf_addr     symval;
		if (sym == 0) {
			symval = 0;
		} else {
			const char    *symname = inst->dynstr + inst->dynsym[sym].name_index;
			if (!find_sym(reloc, symname, &symval)) KBELF_ERROR(abort, "Unable to find symbol %s", symname)
		}
		if (!kbelfp_reloc_apply(file, inst, type, symval, addend, (uint8_t *) laddr)) KBELF_ERROR(abort, "Applying relocation 0x%02x failed", type)
	}
	return true;
	
	abort:
	return false;
}

// Perform the relocation.
// Returns success status.
bool kbelf_reloc_perform(kbelf_reloc reloc) {
	if (!reloc) return false;
	// Iterate objects.
	for (size_t x = 0; x < reloc->libs_len; x++) {
		kbelf_inst inst = reloc->libs_inst[x];
		kbelf_file file = reloc->libs_file[x];
		
		size_t rel_sz  = 0,     rela_sz  = 0;
		size_t rel_ent = 0,     rela_ent = 0;
		void  *rel     = NULL, *rela     = NULL;
		
		// Search for REL and RELA tables.
		for (size_t y = 0; y < inst->dynamic_len; y++) {
			kbelf_dynentry dyn = inst->dynamic[y];
			if (dyn.tag == DT_REL) {
				rel = (void *) kbelf_inst_getladdr(inst, dyn.value);
			} else if (dyn.tag == DT_RELSZ) {
				rel_sz = dyn.value;
			} else if (dyn.tag == DT_RELENT) {
				rel_ent = dyn.value;
			} else if (dyn.tag == DT_RELA) {
				rela = (void *) kbelf_inst_getladdr(inst, dyn.value);
			} else if (dyn.tag == DT_RELASZ) {
				rela_sz = dyn.value;
			} else if (dyn.tag == DT_RELAENT) {
				rela_ent = dyn.value;
			}
		}
		
		// Apply the REL.
		if (rel_sz && rel_ent && rel) {
			if (rel_ent != sizeof(kbelf_relentry)) KBELF_ERROR(abort, "Invalid REL entry size")
			if (!rel_perform(reloc, file, inst, rel_sz/sizeof(kbelf_relentry), rel)) return false;
		}
		
		// Apply the RELA.
		if (rela_sz && rela_ent && rela) {
			if (rela_ent != sizeof(kbelf_relaentry)) KBELF_ERROR(abort, "Invalid RELA entry size")
			if (!rela_perform(reloc, file, inst, rela_sz/sizeof(kbelf_relaentry), rela)) return false;
		}
	}
	
	return true;
	
	abort:
	return false;
}

// Add a loaded instance to a relocation context.
// Returns success status.
bool kbelf_reloc_add(kbelf_reloc reloc, kbelf_file file, kbelf_inst inst) {
	if (!reloc || !file || !inst) return false;
	size_t file_sz = (1+reloc->libs_len) * sizeof(struct struct_kbelf_file);
	size_t inst_sz = (1+reloc->libs_len) * sizeof(struct struct_kbelf_inst);
	void *file_mem = kbelfx_realloc(reloc->libs_file, file_sz);
	void *inst_mem = kbelfx_realloc(reloc->libs_inst, inst_sz);
	if (file_mem) reloc->libs_file = file_mem;
	if (inst_mem) reloc->libs_inst = inst_mem;
	if (!file_mem || !inst_mem) return false;
	reloc->libs_file[reloc->libs_len] = file;
	reloc->libs_inst[reloc->libs_len] = inst;
	reloc->libs_len ++;
	return true;
}

// Add a built-in library to a relocation context.
// Returns success status.
bool kbelf_reloc_add_builtin(kbelf_reloc reloc, const kbelf_builtin_lib *lib) {
	if (!reloc || !lib) return false;
	size_t cap = (1+reloc->builtins_len) * sizeof(kbelf_builtin_lib *);
	void *mem = kbelfx_realloc(reloc->builtins, cap);
	if (!mem) return false;
	reloc->builtins = mem;
	reloc->builtins[reloc->builtins_len] = lib;
	reloc->builtins_len++;
	return true;
}
