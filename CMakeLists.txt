cmake_minimum_required(VERSION 3.15)

project(
    libsrt
    VERSION 0.0.1
    LANGUAGES C
)

include(GNUInstallDirs)
include(CTest)
include(CMakePackageConfigHelpers)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(LIBSRT_MINIMAL "Build with reduced memory usage and binary size" OFF)
option(LIBSRT_DISABLE_VARGS "Disable variadic macro support" OFF)

set(
    LIBSRT_CRC32_SLC
    "12"
    CACHE STRING
    "CRC32 slicing mode: 0, 1, 4, 8, 12, or 16"
)
set_property(CACHE LIBSRT_CRC32_SLC PROPERTY STRINGS 0 1 4 8 12 16)

set(LIBSRT_VALID_CRC32_SLC_VALUES 0 1 4 8 12 16)
if(NOT LIBSRT_CRC32_SLC IN_LIST LIBSRT_VALID_CRC32_SLC_VALUES)
    message(
        FATAL_ERROR
        "LIBSRT_CRC32_SLC must be one of: 0, 1, 4, 8, 12, or 16"
    )
endif()

set(LIBSRT_SOURCES
    src/saux/sdata.c
    src/saux/sdbg.c
    src/saux/senc.c
    src/saux/sstringo.c
    src/saux/schar.c
    src/saux/ssearch.c
    src/saux/ssort.c
    src/saux/stree.c
    src/saux/shash.c
    src/saux/scommon.c
    src/sstring.c
    src/svector.c
    src/smap.c
    src/smset.c
    src/shmap.c
    src/shset.c
    src/sbitset.c
)

add_library(libsrt ${LIBSRT_SOURCES})
add_library(libsrt::libsrt ALIAS libsrt)

set_target_properties(
    libsrt
    PROPERTIES
        OUTPUT_NAME srt
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(
    libsrt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libsrt>
)

target_compile_definitions(
    libsrt
    PRIVATE S_CRC32_SLC=${LIBSRT_CRC32_SLC}
)

if(LIBSRT_MINIMAL)
    target_compile_definitions(libsrt PUBLIC S_MINIMAL)
endif()

if(LIBSRT_DISABLE_VARGS)
    target_compile_definitions(libsrt PUBLIC S_NO_VARGS)
endif()

if(BUILD_TESTING)
    add_executable(stest test/stest.c)
    target_include_directories(stest PRIVATE test)
    target_link_libraries(stest PRIVATE libsrt::libsrt)

    if(UNIX)
        find_library(LIBSRT_M_LIBRARY m)
        if(LIBSRT_M_LIBRARY)
            target_link_libraries(stest PRIVATE ${LIBSRT_M_LIBRARY})
        endif()

        find_library(LIBSRT_RT_LIBRARY rt)
        if(LIBSRT_RT_LIBRARY)
            target_link_libraries(stest PRIVATE ${LIBSRT_RT_LIBRARY})
        endif()
    endif()

    add_test(NAME libsrt.stest COMMAND stest)
endif()

install(
    TARGETS libsrt
    EXPORT libsrtTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libsrt
)

install(
    DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libsrt
    FILES_MATCHING PATTERN "*.h"
)

configure_package_config_file(
    utl/cmake/libsrtConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libsrtConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libsrt
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/libsrtConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    EXPORT libsrtTargets
    FILE libsrtTargets.cmake
    NAMESPACE libsrt::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libsrt
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/libsrtConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/libsrtConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libsrt
)
