project(introspection)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

add_library(introspection SHARED introspection.c
            context.c file.c function.c helper.c mapping.c
            module.c module-elf.c module-pe.c
            process.c section.c syscalls.c syscall-arm.c
            syscall-aarch64.c syscall-i386.c
            syscall-linux.c syscall-linux64.c syscall-win32.c
            syscall-win64.c syscall-x86_64.c)

target_include_directories(introspection PRIVATE ../../qemu/plugins/include)
target_include_directories(introspection PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(introspection INTERFACE ${GLIB_LDFLAGS})

install(TARGETS introspection
        LIBRARY DESTINATION lib)
