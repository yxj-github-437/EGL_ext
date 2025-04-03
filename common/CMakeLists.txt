add_library(common)

set(CMAKE_CXX_STANDARD 17)

target_sources(common PRIVATE logger.cc)
target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(common PUBLIC log)
