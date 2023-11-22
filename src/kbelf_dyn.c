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

// Create a dynamic executable loading context.
// Returns non-null on success, NULL on error.
kbelf_dyn kbelf_dyn_create(int pid) {
    // Allocate memory.
    kbelf_dyn dyn = kbelfx_malloc(sizeof(struct struct_kbelf_dyn));
    if (!dyn)
        KBELF_ERROR(abort, "Out of memory")
    kbelfq_memset(dyn, 0, sizeof(struct struct_kbelf_dyn));

    dyn->pid = pid;
    return dyn;

abort:
    kbelf_dyn_destroy(dyn);
    return NULL;
}

// Clean up a `kbelf_dyn` context.
// Does not unload the process image if it was successfully created.
void kbelf_dyn_destroy(kbelf_dyn dyn) {
    if (!dyn)
        return;
    if (dyn->exec_file)
        kbelf_file_close(dyn->exec_file);
    if (dyn->exec_inst)
        kbelf_inst_destroy(dyn->exec_inst);
    for (size_t i = 0; i < dyn->libs_len; i++) {
        kbelf_file_close(dyn->libs_file[i]);
        kbelf_inst_destroy(dyn->libs_inst[i]);
    }
    if (dyn->libs_len) {
        kbelfx_free(dyn->libs_file);
        kbelfx_free(dyn->libs_inst);
    }
    if (dyn->init_order)
        kbelfx_free(dyn->init_order);
    kbelfx_free(dyn);
}

// Unloads the process image if it was successfully created.
void kbelf_dyn_unload(kbelf_dyn dyn) {
    if (!dyn)
        return;
    kbelf_inst_unload(dyn->exec_inst);
    dyn->exec_inst = NULL;
    for (size_t i = 0; i < dyn->libs_len; i++) {
        kbelf_inst_unload(dyn->libs_inst[i]);
        dyn->libs_inst[i] = NULL;
    }
}

// Set the executable file.
// Returns success status.
bool kbelf_dyn_set_exec(kbelf_dyn dyn, char const *path, void *fd) {
    if (!dyn)
        return false;
    if (dyn->exec_file)
        return false;
    dyn->exec_file = kbelf_file_open(path, fd);
    return dyn->exec_file;
}



// Extract filename from path.
static char const *path_to_filename(char const *path) {
    char const *c0 = kbelfq_strrchr(path, '/');
#if defined(_WIN32) || defined(WIN32)
    char const *c1 = kbelfq_strrchr(path, '\\');
    if (c1 > c0)
        c0 = c1;
#endif
    if (c0)
        path = c0 + 1;
    return path;
}

// Add a library file.
static bool add_lib(kbelf_dyn dyn, kbelf_file file, kbelf_inst inst) {
    if (!dyn || !file)
        return false;
    size_t file_sz  = (1 + dyn->libs_len) * sizeof(struct struct_kbelf_file);
    size_t inst_sz  = (1 + dyn->libs_len) * sizeof(struct struct_kbelf_inst);
    void  *file_mem = kbelfx_realloc(dyn->libs_file, file_sz);
    void  *inst_mem = kbelfx_realloc(dyn->libs_inst, inst_sz);
    if (file_mem)
        dyn->libs_file = file_mem;
    if (inst_mem)
        dyn->libs_inst = inst_mem;
    if (!file_mem || !inst_mem)
        return false;
    dyn->libs_file[dyn->libs_len] = file;
    dyn->libs_inst[dyn->libs_len] = inst;
    dyn->libs_len++;
    return true;
}

// Find a built-in library.
static kbelf_builtin_lib const *find_builtin(char const *needed) {
    for (size_t i = 0; i < kbelfx_builtin_libs_len; i++) {
        if (kbelfq_streq(path_to_filename(needed), path_to_filename(kbelfx_builtin_libs[i].path)))
            return &kbelfx_builtin_libs[i];
    }
    return NULL;
}

// Add a built-in library.
static bool add_builtin(kbelf_dyn dyn, kbelf_builtin_lib const *lib) {
    if (!dyn || !lib)
        return false;
    size_t cap = (1 + dyn->builtins_len) * sizeof(kbelf_builtin_lib *);
    void  *mem = kbelfx_realloc(dyn->builtins, cap);
    if (!mem)
        return false;
    dyn->builtins                      = mem;
    dyn->builtins[dyn->builtins_len++] = lib;
    return true;
}

// Check whether a library is loaded.
static bool check_lib(kbelf_dyn dyn, char const *needed) {
    needed = path_to_filename(needed);
    for (size_t i = 0; i < dyn->builtins_len; i++) {
        if (kbelfq_streq(path_to_filename(dyn->builtins[i]->path), needed))
            return true;
    }
    for (size_t i = 0; i < dyn->libs_len; i++) {
        if (kbelfq_streq(dyn->libs_file[i]->name, needed))
            return true;
    }
    return false;
}

// Check a file for its dependencies and add any missing ones.
static bool check_deps(kbelf_dyn dyn, kbelf_file file, kbelf_inst inst) {
    (void)file;
    for (size_t i = 0; i < inst->dynamic_len; i++) {
        kbelf_dynentry dt = inst->dynamic[i];
        if (dt.tag == DT_NEEDED) {
            char const *needed = inst->dynstr + dt.value;
            if (!check_lib(dyn, needed)) {
                // Check for built-in libs first.
                kbelf_builtin_lib const *builtin = find_builtin(needed);
                if (builtin) {
                    if (!add_builtin(dyn, builtin))
                        KBELF_ERROR(abort, "Out of memory")
                } else {
                    // If built-in fails, check for external libs.
                    kbelf_file lib = kbelfx_find_lib(needed);
                    if (!lib)
                        KBELF_ERROR(abort, "Unable to find " KBELF_FMT_CSTR "", needed)
                    if (!add_lib(dyn, lib, NULL))
                        KBELF_ERROR(abort, "Out of memory")
                }
            }
        }
    }

    return true;

abort:
    return false;
}


// Test whether an instance has init and/or fini functions.
static inline bool has_init_funcs(kbelf_inst inst) {
    return kbelf_inst_preinit_len(inst) || kbelf_inst_init_len(inst) || kbelf_inst_fini_len(inst);
}

// Test whether instance A depends directly or indirectly on instance B.
static bool depends_on_recursive(kbelf_dyn dyn, kbelf_inst a, kbelf_inst b, size_t recursion_limit) {
    if (recursion_limit == 0)
        return true;
    for (size_t i = 0; i < a->dynamic_len; i++) {
        kbelf_dynentry dt = a->dynamic[i];
        if (dt.tag == DT_NEEDED) {
            char const *needed = a->dynstr + dt.value;
            needed             = path_to_filename(needed);
            if (kbelfq_streq(needed, b->name))
                return true;
            for (size_t x = 0; x < dyn->libs_len; x++) {
                if (kbelfq_streq(dyn->libs_inst[x]->name, needed)) {
                    if (depends_on_recursive(dyn, dyn->libs_inst[x], b, recursion_limit - 1))
                        return true;
                }
            }
        }
    }
    return false;
}

// Test whether instance A depends directly or indirectly on instance B.
static inline bool depends_on(kbelf_dyn dyn, kbelf_inst a, kbelf_inst b) {
    return depends_on_recursive(dyn, a, b, dyn->libs_len + 1);
}

// Comparator for sorting function.
static inline int comparator(kbelf_dyn dyn, size_t a, size_t b) {
    kbelf_inst la = dyn->libs_inst[a];
    kbelf_inst lb = dyn->libs_inst[b];
    if (depends_on(dyn, la, lb)) {
        return -1;
    } else if (depends_on(dyn, lb, la)) {
        return 1;
    } else {
        return 0;
    }
}

// Sort the initialisation order.
static void sort_init_order_recursive(kbelf_dyn dyn, size_t *arr, size_t len, size_t *tmp) {
    if (len == 1) {
        // Base case: 1 element.
        return;
    } else if (len == 2) {
        // Base case: 2 elements.
        if (comparator(dyn, arr[0], arr[1]) < 0) {
            size_t tmp = arr[0];
            arr[0]     = arr[1];
            arr[1]     = tmp;
        }
        return;
    }

    // Determine midpoint.
    size_t midpoint = len / 2;

    // Sort left.
    sort_init_order_recursive(dyn, arr, midpoint, tmp);

    // Sort right.
    sort_init_order_recursive(dyn, arr + midpoint, len - midpoint, tmp);

    // Merge.
    for (size_t i = 0, l = 0, r = 0; i < len; i++) {
        if (r < len - midpoint && comparator(dyn, arr[l], arr[r]) < 0) {
            // Take from the right.
            tmp[i] = arr[r++];
        } else {
            // Take from the left.
            tmp[i] = arr[l++];
        }
    }
    kbelfq_memcpy(arr, tmp, sizeof(size_t) * len);
}

// Sort the initialisation order.
static inline bool sort_init_order(kbelf_dyn dyn) {
    if (dyn->init_order_len) {
        size_t *tmp = kbelfx_malloc(sizeof(size_t) * dyn->init_order_len);
        if (!tmp)
            return false;
        sort_init_order_recursive(dyn, dyn->init_order, dyn->init_order_len, tmp);
        kbelfx_free(tmp);
    }
    return true;
}


// Interpret the files and create a process image.
// Returns success status.
bool kbelf_dyn_load(kbelf_dyn dyn) {
    kbelf_reloc reloc = NULL;
    if (!dyn)
        return false;
    if (!dyn->exec_file)
        KBELF_ERROR(abort, "No executable file")

    // Load the executable.
    dyn->exec_inst = kbelf_inst_load(dyn->exec_file, dyn->pid);
    if (!dyn->exec_inst)
        KBELF_ERROR(abort, "Unable to load " KBELF_FMT_CSTR, dyn->exec_file->path)

    // Load libraries for the executable.
    if (!check_deps(dyn, dyn->exec_file, dyn->exec_inst))
        KBELF_ERROR(abort, "Unable to satisfy library requirements")

    // Check dependencies for the libraries.
    for (size_t i = 0; i < dyn->libs_len; i++) {
        if (!dyn->libs_inst[i]) {
            dyn->libs_inst[i] = kbelf_inst_load(dyn->libs_file[i], dyn->pid);
            if (!dyn->libs_inst[i])
                KBELF_ERROR(abort, "Unable to load " KBELF_FMT_CSTR, dyn->libs_file[i]->path)
        }
        if (!check_deps(dyn, dyn->libs_file[i], dyn->libs_inst[i]))
            KBELF_ERROR(abort, "Unable to satisfy library requirements")
    }

    // Count the number of libs with init and/or fini functions.
    for (size_t i = 0; i < dyn->libs_len; i++) {
        dyn->init_order_len += has_init_funcs(dyn->libs_inst[i]);
        dyn->init_len       += kbelf_inst_init_len(dyn->libs_inst[i]);
        dyn->fini_len       += kbelf_inst_fini_len(dyn->libs_inst[i]);
    }

    // Allocate memory.
    dyn->init_order = kbelfx_malloc(sizeof(size_t) * dyn->init_order_len);
    if (!dyn->init_order)
        KBELF_ERROR(abort, "Out of memory")

    // Compute initialisation order.
    for (size_t i = 0, li = 0; i < dyn->libs_len; i++) {
        if (has_init_funcs(dyn->libs_inst[i])) {
            dyn->init_order[li] = i;
            li++;
        }
    }
    if (!sort_init_order(dyn))
        KBELF_ERROR(abort, "Out of memory");

    // Perform relocation.
    reloc = kbelf_reloc_create();
    for (size_t i = 0; i < dyn->builtins_len; i++) {
        if (!kbelf_reloc_add_builtin(reloc, dyn->builtins[i]))
            KBELF_ERROR(abort, "Out of memory")
    }
    if (!kbelf_reloc_add(reloc, dyn->exec_file, dyn->exec_inst))
        KBELF_ERROR(abort, "Out of memory")
    for (size_t i = 0; i < dyn->libs_len; i++) {
        if (!kbelf_reloc_add(reloc, dyn->libs_file[i], dyn->libs_inst[i]))
            KBELF_ERROR(abort, "Out of memory")
    }
    if (!kbelf_reloc_perform(reloc))
        KBELF_ERROR(abort, "Relocation failed")
    kbelf_reloc_destroy(reloc);

    // Success.
    dyn->entrypoint = dyn->exec_inst->entry;
    return true;

// Error.
abort:
    kbelf_reloc_destroy(reloc);
    kbelf_dyn_unload(dyn);
    return false;
}



// Get the number of pre-initialisation functions for the process.
size_t kbelf_dyn_preinit_len(kbelf_dyn dyn) {
    return dyn && dyn->exec_inst ? kbelf_inst_preinit_len(dyn->exec_inst) : 0;
}

// Get the virtual address of an initialisation function by index.
// Functions are sorted; the first index is the first in the running order.
kbelf_addr kbelf_dyn_preinit_get(kbelf_dyn dyn, size_t i) {
    return dyn && dyn->exec_inst ? kbelf_inst_preinit_get(dyn->exec_inst, i) : 0;
}

// Get the number of initialisation functions for the process.
size_t kbelf_dyn_init_len(kbelf_dyn dyn) {
    return dyn ? dyn->init_len : 0;
}

// Get the virtual address of an initialisation function by index.
// Functions are sorted; the first index is the first in the running order.
kbelf_addr kbelf_dyn_init_get(kbelf_dyn dyn, size_t i) {
    if (!dyn || i >= dyn->init_len)
        return 0;

    // Executable first.
    size_t len = kbelf_inst_init_len(dyn->exec_inst);
    if (i < len) {
        return kbelf_inst_init_get(dyn->exec_inst, i);
    }
    i -= len;

    // Libraries second.
    for (size_t x = 0; x < dyn->init_order_len; x++) {
        len = kbelf_inst_init_len(dyn->libs_inst[dyn->init_order[x]]);
        if (i < len)
            return kbelf_inst_init_get(dyn->libs_inst[dyn->init_order[x]], i);
        i -= len;
    }

    return 0;
}

// Get the number of finalisation functions for the process.
size_t kbelf_dyn_fini_len(kbelf_dyn dyn) {
    return dyn ? dyn->fini_len : 0;
}

// Get the virtual address of an finalisation function by index.
// Functions are sorted; the first index is the first in the running order.
kbelf_addr kbelf_dyn_fini_get(kbelf_dyn dyn, size_t i) {
    if (!dyn || i >= dyn->fini_len)
        return 0;

    // Finaliser order is to opposite of initialiser order.
    i = dyn->fini_len - i - 1;

    // Executable first.
    size_t len = kbelf_inst_fini_len(dyn->exec_inst);
    if (i < len) {
        return kbelf_inst_fini_get(dyn->exec_inst, i);
    }
    i -= len;

    // Libraries second.
    for (size_t x = 0; x < dyn->init_order_len; x++) {
        len = kbelf_inst_fini_len(dyn->libs_inst[dyn->init_order[x]]);
        if (i < len)
            return kbelf_inst_fini_get(dyn->libs_inst[dyn->init_order[x]], i);
        i -= len;
    }

    return 0;
}

// Get the entrypoint address of the process.
kbelf_addr kbelf_dyn_entrypoint(kbelf_dyn dyn) {
    return dyn ? dyn->entrypoint : 0;
}
