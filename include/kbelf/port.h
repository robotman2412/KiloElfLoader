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


/* ==== Verification ==== */

// Perform target-specific verification of `kbelf_file`.
bool kbelfp_file_verify(kbelf_file file);


/* ==== Relocation ==== */

// Obtain the value of an implicit addend.
kbelf_addr kbelfp_reloc_get_addend(kbelf_file file, kbelf_inst inst, uint32_t type, uint8_t const *ptr);
// Compute the resulting value of a relocation.
bool       kbelfp_reloc_apply(
          kbelf_file file, kbelf_inst inst, uint32_t type, kbelf_addr sym, kbelf_addr addend, kbelf_laddr laddr
      );



#ifdef __cplusplus
} // extern "C"
#endif
