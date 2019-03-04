project(introspection)

add_library(introspection SHARED introspection.c context.c)

target_include_directories(introspection PRIVATE ../../qemu/plugins/include)

install(TARGETS introspection
        LIBRARY DESTINATION lib)
