cmake_minimum_required(VERSION 3.21)
project(LibraryManagementSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)
include(FetchContent)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if(WIN32)
    set(DUCKDB_ARCHIVE "libduckdb-windows-amd64.zip")
    set(DUCKDB_SHA256 "8fa8b50b49d0ec9a84a95edff9bfa926b8ae336d5d9a3a6442ea494606ebc9c7")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(DUCKDB_ARCHIVE "libduckdb-linux-amd64.zip")
    set(DUCKDB_SHA256 "91b06f8d4bca471bab4f46c676ae2a58c9ca18987d0ec3d25fd95aabaf9eef3e")
else()
    message(FATAL_ERROR "Prebuilt DuckDB packages are configured for Windows and Linux x86_64.")
endif()

FetchContent_Declare(
    duckdb_prebuilt
    URL "https://github.com/duckdb/duckdb/releases/download/v1.2.2/${DUCKDB_ARCHIVE}"
    URL_HASH "SHA256=${DUCKDB_SHA256}"
)
FetchContent_GetProperties(duckdb_prebuilt)
if(NOT duckdb_prebuilt_POPULATED)
    FetchContent_Populate(duckdb_prebuilt)
endif()

add_library(duckdb SHARED IMPORTED GLOBAL)
if(WIN32)
    set_target_properties(duckdb PROPERTIES
        IMPORTED_IMPLIB "${duckdb_prebuilt_SOURCE_DIR}/duckdb.lib"
        IMPORTED_LOCATION "${duckdb_prebuilt_SOURCE_DIR}/duckdb.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${duckdb_prebuilt_SOURCE_DIR}"
    )
else()
    set_target_properties(duckdb PROPERTIES
        IMPORTED_LOCATION "${duckdb_prebuilt_SOURCE_DIR}/libduckdb.so"
        INTERFACE_INCLUDE_DIRECTORIES "${duckdb_prebuilt_SOURCE_DIR}"
        # DuckDB's Linux release is built with the legacy libstdc++ string ABI.
        INTERFACE_COMPILE_DEFINITIONS "_GLIBCXX_USE_CXX11_ABI=0"
    )
endif()

function(copy_duckdb_runtime target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:duckdb>" "$<TARGET_FILE_DIR:${target}>"
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set_property(TARGET ${target} APPEND PROPERTY BUILD_RPATH "$ORIGIN")
    endif()
endfunction()

add_library(lms_core
    src/library.cpp
)
target_include_directories(lms_core PUBLIC include)
target_link_libraries(lms_core PUBLIC duckdb)

add_executable(library-management-system
    src/main.cpp
    src/main_window.cpp
)
target_link_libraries(library-management-system PRIVATE lms_core Qt6::Widgets)
copy_duckdb_runtime(library-management-system)

if(BUILD_TESTING)
    add_executable(lms_tests tests/library_tests.cpp)
    target_link_libraries(lms_tests PRIVATE lms_core)
    copy_duckdb_runtime(lms_tests)
    add_test(NAME library_service COMMAND lms_tests)
endif()
