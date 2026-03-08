add_library(
    io_load_lib
    STATIC
    vtpc.c
    loaders_parser.c
    io_load.c
)

add_executable(io_load main.c)

target_include_directories(
    io_load_lib
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_include_directories(
    io_load
    PUBLIC
    .
)

target_link_libraries(io_load PRIVATE io_load_lib)
