
cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(kbelf)

if("${kbelf_target}" STREQUAL "riscv")
	set(kbelf_port_src src/port/riscv.c)
endif()
if("${kbelf_target}" STREQUAL "x86")
	set(kbelf_port_src src/port/x86.c)
endif()

add_library(kbelf STATIC
	${kbelf_port_src}
	src/kbelf_dyn.c
	src/kbelf_file.c
	src/kbelf_inst.c
	src/kbelf_reloc.c
	src/kbelf.c
)
target_include_directories(kbelf PUBLIC include)
