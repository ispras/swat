project(introspection)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

add_library(introspection SHARED introspection.c
            context.c helper.c
            syscalls.c syscall-i386.c)

target_include_directories(introspection PRIVATE ../../qemu/plugins/include)
target_include_directories(introspection PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(introspection INTERFACE ${GLIB_LDFLAGS})

install(TARGETS introspection
        LIBRARY DESTINATION lib)
