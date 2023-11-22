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



// Helper to determine whether to load a program header.
static bool kbelf_prog_loadable(kbelf_progheader const *prog) {
    return prog->type == PT_LOAD && prog->mem_size;
}

// Load all loadable segments from an ELF file.
// Returns non-null on success, NULL on error.
kbelf_inst kbelf_inst_load(kbelf_file file, int pid) {
    // Allocate memory.
    kbelf_inst inst = kbelfx_malloc(sizeof(struct struct_kbelf_inst));
    if (!inst)
        KBELF_ERROR(abort, "Out of memory")
    kbelfq_memset(inst, 0, sizeof(struct struct_kbelf_inst));
    inst->pid = pid;

    // Copy the path.
    size_t path_len = kbelfq_strlen(file->path);
    inst->path      = kbelfx_malloc(path_len + 1);
    if (!inst->path)
        KBELF_ERROR(abort, "Out of memory")
    kbelfq_memcpy(inst->path, file->path, path_len + 1);
    inst->name = file->name - file->path + inst->path;

    // Count number of loadable segments.
    if (!file->header.ph_ent_num)
        KBELF_ERROR(abort, "No loadable segments")
    size_t loadable_len = 0;
    for (size_t i = 0; i < file->header.ph_ent_num; i++) {
        kbelf_progheader prog = {.type = PT_UNUSED, .mem_size = 0};
        if (!kbelf_file_prog_get(file, &prog, i))
            KBELF_ERROR(abort, "Unable to read program header " KBELF_FMT_SIZE, i)
        loadable_len += kbelf_prog_loadable(&prog);
    }

    // Allocate memory.
    inst->segments_len = loadable_len;
    inst->segments     = kbelfx_malloc(loadable_len * sizeof(kbelf_segment));
    if (!inst->segments)
        KBELF_ERROR(abort, "Out of memory");
    kbelfq_memset(inst->segments, 0, loadable_len * sizeof(kbelf_segment));

    // Interpret program headers.
    for (size_t i = 0, li = 0; li < loadable_len; i++) {
        kbelf_progheader prog = {.type = PT_UNUSED, .mem_size = 0};
        if (!kbelf_file_prog_get(file, &prog, i))
            KBELF_ERROR(abort, "Unable to read program header " KBELF_FMT_SIZE, i)
        if (!kbelf_prog_loadable(&prog))
            continue;
        if (prog.mem_size < prog.file_size)
            KBELF_ERROR(abort, "Invalid program header size")

        // Simple translation of values.
        inst->segments[li].pid       = pid;
        inst->segments[li].vaddr_req = prog.vaddr;
        inst->segments[li].size      = prog.mem_size;
        inst->segments[li].r         = prog.flags & PF_R;
        inst->segments[li].w         = prog.flags & PF_W;
        inst->segments[li].x         = prog.flags & PF_X;
        inst->segments[li].file_off  = (long)prog.offset;
        inst->segments[li].file_size = (long)prog.file_size;

        li++;
    }

    // Allocate memory.
    if (!kbelfx_seg_alloc(inst, inst->segments_len, inst->segments))
        KBELF_ERROR(abort, "Out of virtual memory")

    // Load segments.
    for (size_t i = 0, li = 0; li < loadable_len; i++) {
        kbelf_progheader prog = {.type = PT_UNUSED, .mem_size = 0};
        if (!kbelf_file_prog_get(file, &prog, i))
            KBELF_ERROR(abort, "Unable to read program header " KBELF_FMT_SIZE, i)
        if (!kbelf_prog_loadable(&prog))
            continue;

        // Initialised data.
        if (prog.file_size) {
            int res = kbelfx_seek(file->fd, (long)prog.offset);
            if (res < 0)
                KBELF_ERROR(abort, "I/O error");
            kbelf_laddr laddr = kbelf_inst_paddr_to_laddr(inst, inst->segments[li].paddr);
            res               = kbelfx_read(file->fd, (void *)laddr, (long)prog.file_size);
            if (res < (int)prog.file_size)
                KBELF_ERROR(abort, "I/O error");
        }
        // Zero-initialised data.
        if (prog.file_size < prog.mem_size) {
            kbelf_laddr laddr = kbelf_inst_paddr_to_laddr(inst, inst->segments[li].paddr + prog.file_size);
            kbelfq_memset((void *)laddr, 0, prog.mem_size - prog.file_size);
        }

        li++;
    }

    // Compute entrypoint address.
    if (file->header.entry) {
        inst->entry = kbelf_inst_getvaddr(inst, file->header.entry);
    }

    // Compute dynamic table address.
    for (size_t i = 0; i < file->header.ph_ent_num; i++) {
        kbelf_progheader prog = {.type = PT_UNUSED, .mem_size = 0};
        if (!kbelf_file_prog_get(file, &prog, i))
            KBELF_ERROR(abort, "Unable to read program header " KBELF_FMT_SIZE, i)
        if (prog.type == PT_DYNAMIC) {
            inst->dynamic     = (void *)kbelf_inst_getladdr(inst, prog.vaddr);
            inst->dynamic_len = prog.mem_size / sizeof(kbelf_dynentry);
            break;
        }
    }

    // Parse dynamic table.
    for (size_t i = 0; i < inst->dynamic_len; i++) {
        kbelf_dynentry dt = inst->dynamic[i];
        if (dt.tag == DT_NULL) {
            inst->dynamic_len = i;
            break;
        } else if (dt.tag == DT_SYMTAB) {
            inst->dynsym = (void *)kbelf_inst_getladdr(inst, dt.value);
        } else if (dt.tag == DT_STRTAB) {
            inst->dynstr = (void *)kbelf_inst_getladdr(inst, dt.value);
        } else if (dt.tag == DT_STRSZ) {
            inst->dynstr_len = dt.value;
        } else if (dt.tag == DT_INIT) {
            inst->init_func = kbelf_inst_getvaddr(inst, dt.value);
        } else if (dt.tag == DT_FINI) {
            inst->fini_func = kbelf_inst_getvaddr(inst, dt.value);
        } else if (dt.tag == DT_HASH) {
            kbelf_addr *addr = (void *)kbelf_inst_getladdr(inst, dt.value);
            if (addr != NULL)
                inst->dynsym_len = addr[1];
        } else if (dt.tag == DT_INIT_ARRAY) {
            inst->init_array = kbelf_inst_getvaddr(inst, dt.value);
        } else if (dt.tag == DT_INIT_ARRAYSZ) {
            inst->init_array_len = dt.value / sizeof(kbelf_addr);
        } else if (dt.tag == DT_FINI_ARRAY) {
            inst->fini_array = kbelf_inst_getvaddr(inst, dt.value);
        } else if (dt.tag == DT_FINI_ARRAYSZ) {
            inst->fini_array_len = dt.value / sizeof(kbelf_addr);
        } else if (dt.tag == DT_PREINIT_ARRAY) {
            inst->preinit_array = kbelf_inst_getvaddr(inst, dt.value);
        } else if (dt.tag == DT_PREINIT_ARRAYSZ) {
            inst->preinit_array_len = dt.value / sizeof(kbelf_addr);
        }
    }

    // Assert presence of both length and pointer fields.
    if (!!inst->dynsym ^ !!inst->dynsym_len)
        KBELF_ERROR(abort, "Invalid dynamic section")
    if (!!inst->dynstr ^ !!inst->dynstr_len)
        KBELF_ERROR(abort, "Invalid dynamic section")
    if (!!inst->init_array ^ !!inst->init_array_len)
        KBELF_ERROR(abort, "Invalid dynamic section")
    if (!!inst->fini_array ^ !!inst->fini_array_len)
        KBELF_ERROR(abort, "Invalid dynamic section")
    if (!!inst->preinit_array ^ !!inst->preinit_array_len)
        KBELF_ERROR(abort, "Invalid dynamic section")
    return inst;

abort:
    kbelf_inst_unload(inst);
    return NULL;
}

// Unloads an instance created with `kbelf_load` and clean up the handle.
void kbelf_inst_unload(kbelf_inst inst) {
    if (!inst)
        return;
    if (inst->path)
        kbelfx_free(inst->path);
    if (inst->segments_len) {
        kbelfx_seg_free(inst, inst->segments_len, inst->segments);
        kbelfx_free(inst->segments);
    }
    kbelfx_free(inst);
}

// Clean up the instance handle but not the loaded segments.
void kbelf_inst_destroy(kbelf_inst inst) {
    if (!inst)
        return;
    if (inst->path)
        kbelfx_free(inst->path);
    if (inst->segments_len) {
        kbelfx_free(inst->segments);
    }
    kbelfx_free(inst);
}


// Get the PID number passed when the `kbelf_inst` was created.
int kbelf_inst_getpid(kbelf_inst inst) {
    return inst->pid;
}

// Translate a virtual address to an offset in the file.
long kbelf_inst_getoff(kbelf_inst inst, kbelf_addr vaddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (vaddr >= inst->segments[i].vaddr_req && vaddr < inst->segments[i].vaddr_req + inst->segments[i].size) {
            return (long)vaddr - (long)inst->segments[i].vaddr_req + (long)inst->segments[i].file_off;
        }
    }
    return 0;
}

// Translate a virtual address to a load address in a loaded instance.
// Typically used by an ELF loader/interpreter.
kbelf_laddr kbelf_inst_getladdr(kbelf_inst inst, kbelf_addr vaddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (vaddr >= inst->segments[i].vaddr_req && vaddr < inst->segments[i].vaddr_req + inst->segments[i].size) {
            return (kbelf_laddr)vaddr - (kbelf_laddr)inst->segments[i].vaddr_req + inst->segments[i].laddr;
        }
    }
    return 0;
}

// Translate a virtual address to a physical address in a loaded instance.
kbelf_addr kbelf_inst_getpaddr(kbelf_inst inst, kbelf_addr vaddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (vaddr >= inst->segments[i].vaddr_req && vaddr < inst->segments[i].vaddr_req + inst->segments[i].size) {
            return vaddr - inst->segments[i].vaddr_req + inst->segments[i].paddr;
        }
    }
    return 0;
}

// Translate a virtual address to a virtual address in a loaded instance.
kbelf_addr kbelf_inst_getvaddr(kbelf_inst inst, kbelf_addr vaddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (vaddr >= inst->segments[i].vaddr_req && vaddr < inst->segments[i].vaddr_req + inst->segments[i].size) {
            return vaddr - inst->segments[i].vaddr_req + inst->segments[i].vaddr_real;
        }
    }
    return 0;
}

// Translate a virtual address in a loaded instance to a physical address in a loaded instance.
kbelf_addr kbelf_inst_vaddr_to_paddr(kbelf_inst inst, kbelf_addr vaddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (vaddr >= inst->segments[i].vaddr_real && vaddr < inst->segments[i].vaddr_real + inst->segments[i].size) {
            return vaddr - inst->segments[i].vaddr_real + inst->segments[i].paddr;
        }
    }
    return 0;
}

// Translate a virtual address in a loaded instance to a load address in a loaded instance.
kbelf_laddr kbelf_inst_vaddr_to_laddr(kbelf_inst inst, kbelf_addr vaddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (vaddr >= inst->segments[i].vaddr_real && vaddr < inst->segments[i].vaddr_real + inst->segments[i].size) {
            return (kbelf_laddr)vaddr - (kbelf_laddr)inst->segments[i].vaddr_real + inst->segments[i].laddr;
        }
    }
    return 0;
}

// Translate a physical address in a loaded instance to a virtual address in a loaded instance.
kbelf_addr kbelf_inst_paddr_to_vaddr(kbelf_inst inst, kbelf_addr vaddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (vaddr >= inst->segments[i].paddr && vaddr < inst->segments[i].paddr + inst->segments[i].size) {
            return vaddr - inst->segments[i].paddr + inst->segments[i].vaddr_real;
        }
    }
    return 0;
}

// Translate a physical address in a loaded instance to a load address in a loaded instance.
kbelf_laddr kbelf_inst_paddr_to_laddr(kbelf_inst inst, kbelf_addr vaddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (vaddr >= inst->segments[i].paddr && vaddr < inst->segments[i].paddr + inst->segments[i].size) {
            return (kbelf_laddr)vaddr - (kbelf_laddr)inst->segments[i].paddr + inst->segments[i].laddr;
        }
    }
    return 0;
}

// Translate a load address in a loaded instance to a virtual address in a loaded instance.
kbelf_addr kbelf_inst_laddr_to_vaddr(kbelf_inst inst, kbelf_laddr laddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (laddr >= inst->segments[i].laddr && laddr < inst->segments[i].laddr + inst->segments[i].size) {
            return (kbelf_addr)laddr - (kbelf_addr)inst->segments[i].laddr + inst->segments[i].vaddr_real;
        }
    }
    return 0;
}

// Translate a load address in a loaded instance to a physical address in a loaded instance.
kbelf_addr kbelf_inst_laddr_to_paddr(kbelf_inst inst, kbelf_laddr laddr) {
    if (!inst)
        return 0;
    for (size_t i = 0; i < inst->segments_len; i++) {
        if (laddr >= inst->segments[i].laddr && laddr < inst->segments[i].laddr + inst->segments[i].size) {
            return (kbelf_addr)laddr - (kbelf_addr)inst->segments[i].laddr + inst->segments[i].paddr;
        }
    }
    return 0;
}


// Get the entrypoint address of a loaded instance.
kbelf_addr kbelf_inst_entrypoint(kbelf_inst inst) {
    return inst ? inst->entry : 0;
}

// Get the number of pre-initialisation functions.
size_t kbelf_inst_preinit_len(kbelf_inst inst) {
    return inst ? inst->preinit_array_len : 0;
}

// Get virtual pre-initialisation function address of a loaded instance.
kbelf_addr kbelf_inst_preinit_get(kbelf_inst inst, size_t index) {
    if (!inst)
        return 0;
    kbelf_addr *arr = (void *)kbelf_inst_vaddr_to_laddr(inst, inst->preinit_array);
    return index < inst->preinit_array_len ? arr[index] : 0;
}

// Get the number of initialisation functions.
size_t kbelf_inst_init_len(kbelf_inst inst) {
    return inst ? inst->init_array_len + !!inst->init_func : 0;
}

// Get virtual initialisation function address of a loaded instance.
kbelf_addr kbelf_inst_init_get(kbelf_inst inst, size_t index) {
    if (!inst)
        return 0;
    if (inst->init_func) {
        if (index == 0)
            return inst->init_func;
        index--;
    }
    kbelf_addr *arr = (void *)kbelf_inst_vaddr_to_laddr(inst, inst->init_array);
    return index < inst->init_array_len ? arr[index] : 0;
}

// Get the number of finalisation functions.
size_t kbelf_inst_fini_len(kbelf_inst inst) {
    return inst ? inst->fini_array_len + !!inst->init_func : 0;
}

// Get virtual finalisation function address of a loaded instance.
kbelf_addr kbelf_inst_fini_get(kbelf_inst inst, size_t index) {
    if (!inst)
        return 0;
    if (inst->fini_func) {
        if (index == 0)
            return inst->fini_func;
        index--;
    }
    kbelf_addr *arr = (void *)kbelf_inst_vaddr_to_laddr(inst, inst->fini_array);
    if (arr == NULL)
        return 0;
    return index < inst->fini_array_len ? arr[index] : 0;
}
