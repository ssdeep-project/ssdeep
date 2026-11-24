cmake_minimum_required(VERSION 3.20)
project(ssdeep VERSION 2.15 LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckSymbolExists)

option(BUILD_SHARED_LIBS "Build libfuzzy as a shared library" ON)
option(SSDEEP_LIBONLY "Build only libfuzzy (not ssdeep)" OFF)
option(SSDEEP_BITPARALLEL_STRING_OPS "Use bit-parallel string operations" ON)

set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_YEAR 2026)
set(PACKAGE_VERSION_WINRES "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
set(PACKAGE_VERSION_WINRES_PREREL " (prerelease)")
set(PACKAGE_VERSION_MAN_PREREL " \\- TO BE DETERMINED")
set(PACKAGE_FILEFLAG_WINRES_PREREL 2)

# Keep feature-test macros consistent between the checks and compiled sources.
add_compile_definitions(_FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE=1)
set(CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1)
foreach(header IN ITEMS string.h inttypes.h libgen.h dirent.h stdbit.h fcntl.h
        sys/types.h sys/ioctl.h sys/param.h wchar.h unistd.h sys/stat.h
        sys/disk.h sys/time.h)
    string(TOUPPER "${header}" variable)
    string(REGEX REPLACE "[/.]" "_" variable "${variable}")
    check_include_file("${header}" "HAVE_${variable}")
endforeach()
if(NOT HAVE_INTTYPES_H)
    message(FATAL_ERROR "An inttypes.h header with C99 integer types is required")
endif()

# sys/mount.h requires these prerequisites on FreeBSD.
include(CheckIncludeFiles)
set(mount_headers "")
if(HAVE_SYS_TYPES_H)
    list(APPEND mount_headers sys/types.h)
endif()
if(HAVE_SYS_PARAM_H)
    list(APPEND mount_headers sys/param.h)
endif()
list(APPEND mount_headers sys/mount.h)
check_include_files("${mount_headers}" HAVE_SYS_MOUNT_H)
check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)
check_symbol_exists(ftello "stdio.h" HAVE_FTELLO)
if(NOT HAVE_FSEEKO OR NOT HAVE_FTELLO)
    message(FATAL_ERROR "The platform must provide fseeko and ftello")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(__LINUX__ 1)
endif()
if(NOT SSDEEP_BITPARALLEL_STRING_OPS)
    set(FUZZY_DISABLE_POSITION_ARRAY 1)
endif()
configure_file(cmake/config.h.in config.h)
configure_file(ssdeep.1.in ssdeep.1 @ONLY)

add_library(fuzzy src/fuzzy.c src/edit_dist.c)
target_compile_features(fuzzy PRIVATE c_std_99)
if(MINGW)
    target_link_options(fuzzy PRIVATE -static-libgcc)
endif()
target_compile_definitions(fuzzy PRIVATE HAVE_CONFIG_H)
target_include_directories(fuzzy PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
# libtool's 3:0:1 version-info corresponds to ABI 2, version 2.1.0.
set_target_properties(fuzzy PROPERTIES VERSION 2.1.0 SOVERSION 2
    WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(WIN32)
    enable_language(RC)
    add_compile_definitions(UNICODE _UNICODE)
    configure_file(fuzzy-win32res.rc.in fuzzy-win32res.rc @ONLY)
    if(BUILD_SHARED_LIBS)
        target_sources(fuzzy PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/fuzzy-win32res.rc")
        set_target_properties(fuzzy PROPERTIES PREFIX "")
        if(MINGW)
            target_link_options(fuzzy PRIVATE
                "-Wl,--output-def,${CMAKE_CURRENT_BINARY_DIR}/fuzzy.def")
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fuzzy.def"
                DESTINATION "${CMAKE_INSTALL_LIBDIR}")
        endif()
    endif()
endif()

if(NOT SSDEEP_LIBONLY)
    enable_language(CXX)
    add_executable(ssdeep src/main.cpp src/match.cpp src/engine.cpp src/filedata.cpp
        src/dig.cpp src/cycles.cpp src/helpers.cpp src/ui.cpp src/find-file-size.c)
    target_compile_features(ssdeep PRIVATE cxx_std_11)
    target_compile_definitions(ssdeep PRIVATE HAVE_CONFIG_H)
    target_include_directories(ssdeep PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_link_libraries(ssdeep PRIVATE fuzzy)
    if(MINGW)
        target_link_options(ssdeep PRIVATE -static)
    endif()
    if(WIN32)
        configure_file(ssdeep-win32res.rc.in ssdeep-win32res.rc @ONLY)
        target_sources(ssdeep PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/ssdeep-win32res.rc")
    endif()
    if(UNIX)
        file(RELATIVE_PATH library_path "${CMAKE_INSTALL_FULL_BINDIR}"
            "${CMAKE_INSTALL_FULL_LIBDIR}")
        if(APPLE)
            set_target_properties(ssdeep PROPERTIES INSTALL_RPATH "@loader_path/${library_path}")
        else()
            set_target_properties(ssdeep PROPERTIES INSTALL_RPATH "$ORIGIN/${library_path}")
        endif()
    endif()
    install(TARGETS ssdeep RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ssdeep.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
endif()

install(TARGETS fuzzy
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES src/fuzzy.h src/edit_dist.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES README.md AUTHORS.md NEWS.md ChangeLog.md FILEFORMAT.md INSTALL.md
    TODO.md COPYING src/sample.c DESTINATION "${CMAKE_INSTALL_DOCDIR}")

include(CTest)
if(BUILD_TESTING)
    add_executable(fuzzy-test src/tests/fuzzy-test.c)
    target_link_libraries(fuzzy-test PRIVATE fuzzy)
    add_test(NAME fuzzy-api COMMAND fuzzy-test)
    if(NOT SSDEEP_LIBONLY)
        add_test(NAME ssdeep-version COMMAND ssdeep -V)
        set_tests_properties(ssdeep-version PROPERTIES
            PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")
        add_test(NAME ssdeep-hash COMMAND ssdeep "${CMAKE_CURRENT_SOURCE_DIR}/src/fuzzy.c")
        set_tests_properties(ssdeep-hash PROPERTIES
            PASS_REGULAR_EXPRESSION "ssdeep,1.1--blocksize:hash:hash,filename")
    endif()
endif()

set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "floss_ssdeep@irq.a4lg.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_GENERATOR ZIP)
set(CPACK_SOURCE_IGNORE_FILES "/[.]git/;/build[^/]*/;[~]$;${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)
