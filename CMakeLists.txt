
cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(kbelf)

add_library(kbelf STATIC
	src/port/riscv.c
	src/kbelf_dyn.c
	src/kbelf_file.c
	src/kbelf_inst.c
	src/kbelf_reloc.c
	src/kbelf.c
)
target_include_directories(kbelf PUBLIC include)
