project(exec-log)

add_library(exec-log SHARED exec-log.c)

target_include_directories(exec-log PRIVATE ../../qemu/plugins/include)

install(TARGETS exec-log
        LIBRARY DESTINATION lib)
