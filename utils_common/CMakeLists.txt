add_library(utils_common)

set(CMAKE_CXX_STANDARD 17)

target_sources(utils_common PRIVATE logger.cc)
target_include_directories(utils_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(utils_common PROPERTIES POSITION_INDEPENDENT_CODE ON
    INTERFACE_POSITION_INDEPENDENT_CODE ON)

if(ANDROID)
    target_link_libraries(utils_common PUBLIC log)
endif()

if (WITH_HYBRIS)
    target_compile_definitions(utils_common PUBLIC __HYBRIS__)
    target_link_libraries(utils_common PUBLIC hybris-common)
endif()
