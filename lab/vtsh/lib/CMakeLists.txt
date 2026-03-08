add_library(
    libvtsh
    STATIC
    vtsh.c
    command_parser.c
    command_executor.c
    loaders_parser.c
)

target_include_directories(
    libvtsh
    PUBLIC
    .
)
