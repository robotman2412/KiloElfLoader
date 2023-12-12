
#include <stdio.h>
#include <stdlib.h>

#include <kbelf.h>

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
// Takes a segment with requested address and permissions and returns a segment with physical and virtual address
// information. If `paddr` is zero the allocation has failed. User-defined.
bool kbelfx_seg_alloc(kbelf_inst inst, size_t segs_len, kbelf_segment *segs) {
    if (!segs_len)
        return false;

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
    if (!mem)
        return false;

    // Compute segment addresses.
    for (size_t i = 0; i < segs_len; i++) {
        kbelf_addr laddr     = (kbelf_laddr)mem + segs[i].vaddr_req - addr_min;
        segs[i].alloc_cookie = NULL;
        segs[i].laddr        = laddr;
        segs[i].vaddr_real   = segs[i].vaddr_req;
    }
    segs[0].alloc_cookie = mem;
}

// Memory allocator function to use for loading program segments.
// Takes a previously allocated segment and unloads it.
// User-defined.
void kbelfx_seg_free(kbelf_inst inst, size_t segs_len, kbelf_segment *segs) {
    if (!segs_len)
        return;
    free(segs[0].alloc_cookie);
}


// Open a binary file for reading.
// User-defined.
void *kbelfx_open(char const *path) {
    return fopen(path, "rb");
}

// Close a file.
// User-defined.
void kbelfx_close(void *fd) {
    fclose((FILE *)fd);
}

// Reads a single byte from a file.
// Returns byte on success, -1 on error.
// User-defined.
int kbelfx_getc(void *fd) {
    return fgetc((FILE *)fd);
}

// Reads a number of bytes from a file.
// Returns the number of bytes read, or less than that on error.
// User-defined.
int kbelfx_read(void *fd, void *buf, int buf_len) {
    return fread(buf, 1, buf_len, (FILE *)fd);
}

// Sets the absolute offset in the file.
// Returns >=0 on success, -1 on error.
int kbelfx_seek(void *fd, long pos) {
    return fseek((FILE *)fd, pos, SEEK_SET);
}


// Find and open a dynamic library file.
// Returns non-null on success, NULL on error.
// User-defined.
kbelf_file kbelfx_find_lib(char const *needed) {
    return kbelf_file_open(needed, NULL);
}

// Number of built-in libraries.
// Optional user-defined.
size_t                   kbelfx_builtin_libs_len = 1;
// Array of built-in libraries.
// Optional user-defined.
kbelf_builtin_lib const *kbelfx_builtin_libs     = (kbelf_builtin_lib const[]
){{.path        = "libstub0.elf",
       .symbols_len = 1,
       .symbols     = (kbelf_builtin_sym const[]){
       {"pront", (kbelf_addr)&abi_pront, (kbelf_addr)&abi_pront},
   }}};
