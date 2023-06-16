# KiloElfLoader
KiloElfLoader, or KBELF for short, is a minimal dynamic ELF loader library intended to be used for small device operating systems.
Because of this usecase, it does not require the C standard library at all, instead opting to have the user of KBELF implement some of the functions.

# Documentation
- I'm working on it.

# Getting started
*Note: KBELF is for OS developers and uses the relevant terminology. Be aware that I assume you have quite a lot of knowledge about how operating systems work.*

## 1. Implementing the system library
Because KBELF does not explicitly rely on libc, it instead opts to declare, but not implement, a few functions which replace the functionality.
In the top of `<kbelf.h>`, there are a few functions with prefix `kbelfx_`. You need to implement these functions in your own code to be able to use KBELF. An example based on libc is included in `examples/kbelfx_libc.c`.

*Note: While KBELF does not reference any of the symbols from libc, it does make use of `<stdint.h>`, `<stdbool.h>` and `<stddef.h>` for type definitions.*

## 2. Creating compatible object files for loading
Unless your OS implements virtual memory, you must compile object files as position-independent code (`-fpic` or `-fPIC` option).
You *may* compile them as `-static-pie` objects, but this might severely restrict how your OS can implement its system calls.

## 3. Usage of the dynamic loader API
The actual usage of the API is relatively simple.
Below is a minimal example code which loads a dynamic executable `a.out`:
```c
// Create a dynamic loader context.
// The number given is the process ID; it is purely informational.
kbelf_dyn dyn = kbelf_dyn_create(1);
// Specify the executable file to load.
kbelf_dyn_set_exec(dyn, "a.out", NULL);
// Attempt to load the executable and create a process image.
kbelf_dyn_perform(dyn);
```

If all three functions return `true`/nonnull, you have successfully loaded a dynamic executable and its library dependencies.
You may now make use of the other `kbelf_dyn_` functions to get the information required to start the executable.

*Note: KBELF uses opaque handle types for some of its functions to avoid breaking your OS if the type's layout changes. This is the case for `kbelf_file`, `kbelf_inst`, `kbelf_reloc` and `kbelf_dyn`.*

To clean up the things you just loaded:
```c
// Deletes the process image (but not the loader context).
kbelf_dyn_unload(dyn);
// Deletes the loader context (but not the process image).
kbelf_dyn_destroy(dyn);
```

*Note: There is a separate function for unloading the process image versus cleaning up the context that created it. This is to allow an operating system to choose to reclaim the memory from KBELF while the process may still be running.*

# Support
KBELF currently has a very narrow target window: 32-bit RISC-V.
In the future, I may expand this to 64-bit RISC-V and maybe even x86 and x86_64.

## Supported object files
- ELF (32-bit; endianness must match that of the host device)

## Supported architectures
- RISC-V 32-bit (RV32E and RV32I)

## Planned support in the future
- RISC-V 64-bit (RV64I; unsupported due to ELF64 being unsupported)
