
#include <kbelf.h>
#include <stdlib.h>
#include <stdio.h>

#define SINGLE_ALLOC

// Memory allocator function to use for allocating metadata.
// User-defined.
void *kbelfx_malloc(size_t len) {
	return malloc(len);
}

// Memory allocator function to use for allocating metadata.
// User-defined.
void *kbelfx_realloc(void *mem, size_t len) {
	return realloc(mem, len);
}

// Memory allocator function to use for allocating metadata.
// User-defined.
void kbelfx_free(void *mem) {
	free(mem);
}


// Memory allocator function to use for loading program segments.
// Takes a segment with requested address and permissions and returns a segment with physical and virtual address information.
// If `paddr` is zero the allocation has failed.
// User-defined.
bool kbelfx_seg_alloc(size_t segs_len, kbelf_segment *segs) {
	if (!segs_len) return false;
	
	// Determine required size.
	kbelf_addr addr_min = -1;
	kbelf_addr addr_max = 0;
	for (size_t i = 0; i < segs_len; i++) {
		if (segs[i].vaddr_req < addr_min) {
			addr_min = segs[i].vaddr_req;
		}
		if (segs[i].vaddr_req + segs[i].size > addr_max) {
			addr_max = segs[i].vaddr_req + segs[i].size;
		}
	}
	
	// Allocate memory.
	void *mem = malloc(addr_max - addr_min);
	if (!mem) return false;
	
	// Compute segment addresses.
	for (size_t i = 0; i < segs_len; i++) {
		kbelf_addr addr = (kbelf_addr) mem + segs[i].vaddr_req - addr_min;
		segs[i].alloc_cookie = NULL;
		segs[i].paddr        = addr;
		segs[i].vaddr_real   = addr;
	}
	segs[0].alloc_cookie = mem;
}

// Memory allocator function to use for loading program segments.
// Takes a previously allocated segment and unloads it.
// User-defined.
void kbelfx_seg_free(size_t segs_len, kbelf_segment *segs) {
	if (!segs_len) return;
	#ifdef SINGLE_ALLOC
	free(segs[0].alloc_cookie);
	#else
	for (size_t i = 0; i < segs_len; i++) {
		free(segs[i].alloc_cookie);
	}
	#endif
}


// Open a binary file for reading.
// User-defined.
void *kbelfx_open(const char *path) {
	return fopen(path, "rb");
}

// Close a file.
// User-defined.
void kbelfx_close(void *fd) {
	fclose((FILE *) fd);
}

// Reads a single byte from a file.
// Returns byte on success, -1 on error.
// User-defined.
int kbelfx_getc(void *fd) {
	return fgetc((FILE *) fd);
}

// Reads a number of bytes from a file.
// Returns the number of bytes read, or less than that on error.
// User-defined.
int kbelfx_read(void *fd, void *buf, int buf_len) {
	return fread(buf, 1, buf_len, (FILE *) fd);
}

// Sets the absolute offset in the file.
// Returns >=0 on success, -1 on error.
int kbelfx_seek(void *fd, long pos) {
	return fseek((FILE *) fd, pos, SEEK_SET);
}


// Find and open a dynamic library file.
// Returns non-null on success, NULL on error.
// User-defined.
kbelf_file kbelfx_find_lib(const char *needed) {
	return kbelf_file_open(needed, NULL);
}


// Translate a physical address into a virtual address.
// Returns nonzero on success, 0 on error.
// User-defined.
kbelf_addr kbelfx_paddr_to_vaddr(kbelf_inst inst, kbelf_addr paddr) {
	return paddr;
}

// Translate a virtual address into a physical address.
// Returns nonzero on success, 0 on error.
// User-defined.
kbelf_addr kbelfx_vaddr_to_paddr(kbelf_inst inst, kbelf_addr vaddr) {
	return vaddr;
}


// Very simple printer built-in function.
static void abi_pront(const char *strstr) {
	fputs(strstr, stdout);
}

// Number of built-in libraries.
// Optional user-defined.
size_t kbelfx_builtin_libs_len = 1;
// Array of built-in libraries.
// Optional user-defined.
const kbelf_builtin_lib *kbelfx_builtin_libs = (const kbelf_builtin_lib[]) {
	{
		.path = "libstub0.elf",
		.symbols_len = 1,
		.symbols = (const kbelf_builtin_sym[]) {
			{ "pront", (kbelf_addr) &abi_pront, (kbelf_addr) &abi_pront },
		}
	}
};
