add_library(utils_common)

set(CMAKE_CXX_STANDARD 17)

target_sources(utils_common PRIVATE logger.cc)
target_include_directories(utils_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    target_link_libraries(utils_common PUBLIC log)
endif()
