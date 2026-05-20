cmake_minimum_required(VERSION 3.16)

project(galay-tracing VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(cmake/option.cmake)
include(CTest)

add_subdirectory(galay-tracing)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(BUILD_BENCHMARKS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark/CMakeLists.txt")
    add_subdirectory(benchmark)
endif()

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/galay-tracing-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/galay-tracing-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/galay-tracing-config.cmake"
    INSTALL_DESTINATION lib/cmake/galay-tracing
)

install(
    EXPORT galay-tracing-targets
    FILE galay-tracing-targets.cmake
    NAMESPACE galay-tracing::
    DESTINATION lib/cmake/galay-tracing
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/galay-tracing-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/galay-tracing-config-version.cmake"
    DESTINATION lib/cmake/galay-tracing
)
