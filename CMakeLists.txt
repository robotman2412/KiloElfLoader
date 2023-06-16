
cmake_minimum_required(VERSION 3.10.0)

project(kbelf)

add_library(kbelf STATIC
	src/port/riscv.c
	src/kbelf_file.c
	src/kbelf_inst.c
	src/kbelf_reloc.c
	src/kbelf_dyn.c
	src/kbelf.c
)
target_include_directories(kbelf PUBLIC include)
