add_library(
    vt
    STATIC
    cmp_file.cpp
    exception.cpp
    file.cpp
    log_file.cpp
)

target_include_directories(vt PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(vt PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(vt PRIVATE io_load_lib)
