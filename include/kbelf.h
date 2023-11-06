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

#include <kbelf/types.h>

#ifdef __cplusplus
extern "C" {
#endif



/* ==== User-defined ==== */

// Memory allocator function to use for allocating metadata.
// User-defined.
extern void *kbelfx_malloc(size_t len);
// Memory allocator function to use for allocating metadata.
// User-defined.
extern void *kbelfx_realloc(void *mem, size_t len);
// Memory allocator function to use for allocating metadata.
// User-defined.
extern void  kbelfx_free(void *mem);

// Memory allocator function to use for loading program segments.
// Takes a segment with requested address and permissions and returns a segment with physical and virtual address
// information. Returns success status. User-defined.
extern bool kbelfx_seg_alloc(kbelf_inst inst, size_t segs_len, kbelf_segment *segs);
// Memory allocator function to use for loading program segments.
// Takes a previously allocated segment and unloads it.
// User-defined.
extern void kbelfx_seg_free(kbelf_inst inst, size_t segs_len, kbelf_segment *segs);

// Open a binary file for reading.
// User-defined.
extern void *kbelfx_open(char const *path);
// Close a file.
// User-defined.
extern void  kbelfx_close(void *fd);
// Reads a single byte from a file.
// Returns byte on success, -1 on error.
// User-defined.
extern int   kbelfx_getc(void *fd);
// Reads a number of bytes from a file.
// Returns the number of bytes read, or less than that on error.
// User-defined.
extern int   kbelfx_read(void *fd, void *buf, int buf_len);
// Sets the absolute offset in the file.
// Returns 0 on success, -1 on error.
// User-defined.
extern int   kbelfx_seek(void *fd, long pos);

// Find and open a dynamic library file.
// Returns non-null on success, NULL on error.
// User-defined.
extern kbelf_file kbelfx_find_lib(char const *needed);

// Number of built-in libraries.
// Optional user-defined.
extern size_t                   kbelfx_builtin_libs_len;
// Array of built-in libraries.
// Optional user-defined.
extern kbelf_builtin_lib const *kbelfx_builtin_libs;



/* ==== ELF file interpretation ==== */

// Create a context for interpreting an ELF file.
// The `fd` argument is saved and passed to the file I/O functions.
// If `fd` is NULL, `kbelfx_open` is called with `path`.
// KBELF calls `kbelfx_close` on `fd` when `kbelf_file_close` is called on an `kbelf_file` or when `kbelf_file_open`
// fails. Returns non-null on success, NULL on error.
kbelf_file kbelf_file_open(char const *path, void *fd);
// Clean up a `kbelf_file` context.
// Calls `kbelfx_close` on the `fd` originally provided to `kbelf_file_open`.
void       kbelf_file_close(kbelf_file file);

// Get the number of program headers in an ELF file.
// Returns 0 when there are no program headers.
size_t kbelf_file_prog_len(kbelf_file file) __attribute__((pure));
// Get a copy of a program header in an ELF file using a pointer.
// Returns success status.
bool   kbelf_file_prog_get(kbelf_file file, kbelf_progheader *prog, size_t index);



/* ==== Loading ==== */

// Load all loadable segments from an ELF file.
// The `pid` number is passed to `kbelfx_seg_alloc` and is otherwise ignored.
// Returns non-null on success, NULL on error.
kbelf_inst  kbelf_inst_load(kbelf_file file, int pid);
// Unloads an instance created with `kbelf_load` and clean up the handle.
void        kbelf_inst_unload(kbelf_inst inst);
// Clean up the instance handle but not the loaded segments.
void        kbelf_inst_destroy(kbelf_inst inst);
// Get the PID number passed when the `kbelf_inst` was created.
int         kbelf_inst_getpid(kbelf_inst inst);
// Translate a virtual address to a load address in a loaded instance.
// Typically used by an ELF loader/interpreter.
kbelf_laddr kbelf_inst_getladdr(kbelf_inst inst, kbelf_addr vaddr) __attribute__((pure));
// Translate a virtual address to a physical address in a loaded instance.
// Typically used by the kernel.
kbelf_addr  kbelf_inst_getpaddr(kbelf_inst inst, kbelf_addr vaddr) __attribute__((pure));
// Translate a virtual address to a virtual address in a loaded instance.
// Typically used by the application.
kbelf_addr  kbelf_inst_getvaddr(kbelf_inst inst, kbelf_addr vaddr) __attribute__((pure));
// Translate a virtual address in a loaded instance to a physical address in a loaded instance.
kbelf_addr  kbelf_inst_vaddr_to_paddr(kbelf_inst inst, kbelf_addr vaddr) __attribute__((pure));
// Translate a virtual address in a loaded instance to a load address in a loaded instance.
kbelf_laddr kbelf_inst_vaddr_to_laddr(kbelf_inst inst, kbelf_addr vaddr) __attribute__((pure));
// Translate a physical address in a loaded instance to a virtual address in a loaded instance.
kbelf_addr  kbelf_inst_paddr_to_vaddr(kbelf_inst inst, kbelf_addr vaddr) __attribute__((pure));
// Translate a physical address in a loaded instance to a load address in a loaded instance.
kbelf_laddr kbelf_inst_paddr_to_laddr(kbelf_inst inst, kbelf_addr vaddr) __attribute__((pure));
// Translate a load address in a loaded instance to a virtual address in a loaded instance.
kbelf_addr  kbelf_inst_laddr_to_vaddr(kbelf_inst inst, kbelf_laddr vaddr) __attribute__((pure));
// Translate a load address in a loaded instance to a physical address in a loaded instance.
kbelf_addr  kbelf_inst_laddr_to_paddr(kbelf_inst inst, kbelf_laddr vaddr) __attribute__((pure));
// Get the virtual entrypoint address of a loaded instance.
kbelf_addr  kbelf_inst_entrypoint(kbelf_inst inst) __attribute__((pure));
// Get the number of pre-initialisation functions.
size_t      kbelf_inst_preinit_len(kbelf_inst inst) __attribute__((pure));
// Get virtual pre-initialisation function address of a loaded instance.
kbelf_addr  kbelf_inst_preinit_get(kbelf_inst inst, size_t index) __attribute__((pure));
// Get the number of initialisation functions.
size_t      kbelf_inst_init_len(kbelf_inst inst) __attribute__((pure));
// Get virtual initialisation function address of a loaded instance.
kbelf_addr  kbelf_inst_init_get(kbelf_inst inst, size_t index) __attribute__((pure));
// Get the number of finalisation functions.
size_t      kbelf_inst_fini_len(kbelf_inst inst) __attribute__((pure));
// Get virtual finalisation function address of a loaded instance.
kbelf_addr  kbelf_inst_fini_get(kbelf_inst inst, size_t index) __attribute__((pure));



/* ==== Relocation ==== */

// Create an empty relocation context.
kbelf_reloc kbelf_reloc_create();
// Clean up a `kbelf_reloc` context.
void        kbelf_reloc_destroy(kbelf_reloc reloc);
// Perform the relocation.
// Returns success status.
bool        kbelf_reloc_perform(kbelf_reloc reloc);
// Add a loaded instance to a relocation context.
// Returns success status.
bool        kbelf_reloc_add(kbelf_reloc reloc, kbelf_file file, kbelf_inst inst);
// Add a built-in library to a relocation context.
// Returns success status.
bool        kbelf_reloc_add_builtin(kbelf_reloc reloc, kbelf_builtin_lib const *lib);



/* ==== Executable loading ==== */

// Create a dynamic executable loading context.
// The `pid` number is passed to `kbelfx_seg_alloc` and is otherwise ignored.
// Returns non-null on success, NULL on error.
kbelf_dyn  kbelf_dyn_create(int pid);
// Clean up a `kbelf_dyn` context.
// Does not unload the process image if it was successfully created.
void       kbelf_dyn_destroy(kbelf_dyn dyn);
// Set the executable file.
// Returns success status.
bool       kbelf_dyn_set_exec(kbelf_dyn dyn, char const *path, void *fd);
// Interpret the files and create a process image.
// Returns success status.
bool       kbelf_dyn_load(kbelf_dyn dyn);
// Unloads the process image if it was successfully created.
void       kbelf_dyn_unload(kbelf_dyn dyn);
// Get the virtual entrypoint address of the process.
kbelf_addr kbelf_dyn_entrypoint(kbelf_dyn dyn) __attribute__((pure));
// Get the number of pre-initialisation functions for the process.
size_t     kbelf_dyn_preinit_len(kbelf_dyn dyn) __attribute__((pure));
// Get the virtual address of an initialisation function by index.
// Functions are sorted; the first index is the first in the running order.
kbelf_addr kbelf_dyn_preinit_get(kbelf_dyn dyn, size_t index) __attribute__((pure));
// Get the number of pre-initialisation functions for the process.
size_t     kbelf_dyn_init_len(kbelf_dyn dyn) __attribute__((pure));
// Get the virtual address of an initialisation function by index.
// Functions are sorted; the first index is the first in the running order.
kbelf_addr kbelf_dyn_init_get(kbelf_dyn dyn, size_t index) __attribute__((pure));
// Get the number of finalisation functions for the process.
size_t     kbelf_dyn_fini_len(kbelf_dyn dyn) __attribute__((pure));
// Get the virtual address of an finalisation function by index.
// Functions are sorted; the first index is the first in the running order.
kbelf_addr kbelf_dyn_fini_get(kbelf_dyn dyn, size_t index) __attribute__((pure));



#ifdef __cplusplus
} // extern "C"
#endif
