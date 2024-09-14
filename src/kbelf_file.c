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

// Create a context for interpreting an ELF file.
// The `fd` argument is saved and passed to the file I/O functions.
// If `fd` is NULL, `kbelfx_open` is called with `path`.
// KBELF calls `kbelfx_close` on `fd` when `kbelf_file_close` is called on an `kbelf_file` or when `kbelf_file_open`
// fails. Returns non-null on success, NULL on error.
kbelf_file kbelf_file_open(char const *path, void *fd) {
    // Allocate memories.
    kbelf_file file = kbelfx_malloc(sizeof(struct struct_kbelf_file));
    if (!file)
        KBELF_ERROR(abort, "Out of memory")
    kbelfq_memset(file, 0, sizeof(struct struct_kbelf_file));

    // Try to open file handle.
    if (!fd) {
        fd = kbelfx_open(path);
        if (!fd)
            KBELF_ERROR(abort, "File not found: " KBELF_FMT_CSTR, path)
    }
    file->fd = fd;


    // Create a copy of path.
    size_t path_len = kbelfq_strlen(path);
    file->path      = kbelfx_malloc(path_len + 1);
    if (!file->path)
        KBELF_ERROR(abort, "Out of memory")
    kbelfq_strcpy(file->path, path);

    // Find filename.
    char const *c0 = kbelfq_strrchr(file->path, '/');
#if defined(_WIN32) || defined(WIN32)
    char const *c1 = kbelfq_strrchr(file->path, '\\');
    if (c1 > c0)
        c0 = c1;
#endif
    file->name = c0 ? c0 + 1 : file->path;


    // Load header.
    long len = kbelfx_read(file->fd, &file->header, sizeof(file->header));
    if (len != sizeof(file->header))
        KBELF_ERROR(
            abort,
            "I/O error: expected " KBELF_FMT_SIZE " bytes, got " KBELF_FMT_SIZE " bytes",
            sizeof(file->header),
            (size_t)len
        )


    // Validate header.
    if (!kbelfq_memeq(file->header.magic, kbelf_magic, 4))
        KBELF_ERROR(abort, "Invalid magic")
    if (file->header.word_size != KBELF_CLASS)
        KBELF_ERROR(abort, "Invalid or unsupported class")
    if (file->header.endianness != KBELF_ENDIANNESS)
        KBELF_ERROR(abort, "Invalid or unsupported endianness")
    if (file->header.version != 1)
        KBELF_ERROR(abort, "Invalid or unsupported version")

    if (file->header.type != ET_DYN && file->header.type != ET_EXEC)
        KBELF_ERROR(abort, "Unsupported type")
    if (file->header.machine != kbelf_machine_type)
        KBELF_ERROR(abort, "Unsupported machine")
    if (file->header.version2 != 1)
        KBELF_ERROR(abort, "Invalid or unsupported version2")

    if (file->header.size != sizeof(file->header))
        KBELF_ERROR(abort, "Invalid header size")
    if (file->header.ph_ent_size != sizeof(kbelf_progheader))
        KBELF_ERROR(abort, "Invalid program header entry size")
    if (file->header.sh_ent_size != sizeof(kbelf_sectheader))
        KBELF_ERROR(abort, "Invalid section header entry size")

    // Architecture-specific verification.
    if (!kbelfp_file_verify(file))
        goto abort;

    // Successfully opened.
    return file;

// Some sort of error occurred.
abort:
    kbelf_file_close(file);
    return NULL;
}

// Get the file descriptor in use by the `kbelf_file`.
void *kbelf_file_getfd(kbelf_file file) {
    return file->fd;
}

// Clean up a `kbelf_file` context.
// Calls `kbelfx_close` on the `fd` originally provided to `kbelf_file_open`.
void kbelf_file_close(kbelf_file file) {
    if (!file)
        return;
    if (file->strtab)
        kbelfx_free(file->strtab);
    if (file->path)
        kbelfx_free(file->path);
    if (file->fd)
        kbelfx_close(file->fd);
    kbelfx_free(file);
}


// Get a pointer to the ELF file header.
kbelf_header const *kbelf_file_header(kbelf_file file) {
    return &file->header;
}

// Returns the number of program headers in an ELF file.
// Returns 0 when there are no program headers.
size_t kbelf_file_prog_len(kbelf_file file) {
    return file ? file->header.ph_ent_num : 0;
}

// Returns a copy of a program header in an ELF file using a pointer.
// Returns success status.
bool kbelf_file_prog_get(kbelf_file file, kbelf_progheader *prog, size_t index) {
    if (!file)
        return false;
    if (index > file->header.ph_ent_num)
        return false;
    long res = kbelfx_seek(file->fd, (long)file->header.ph_offset + (long)sizeof(kbelf_progheader) * (long)index);
    if (res < 0)
        return false;
    res = kbelfx_read(file->fd, prog, sizeof(kbelf_progheader));
    if (res < (long)sizeof(kbelf_progheader))
        return false;
    return true;
}
