project(sync_stub)

add_library(sync SHARED
    src/sync_stub.c)

target_include_directories(sync PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
