add_executable(
    shell
    main.c
)

target_include_directories(
    shell
    PUBLIC
    .
)

target_link_libraries(
    shell
    PRIVATE
    libvtsh
)

add_custom_command(TARGET shell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:shell> ${CMAKE_SOURCE_DIR}/test/
)
