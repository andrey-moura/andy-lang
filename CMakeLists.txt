#Require a minimum version
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" ANDYLANG_PROJECT_VERSION)
string(STRIP "${ANDYLANG_PROJECT_VERSION}" ANDYLANG_PROJECT_VERSION)
set(ANDYLANG_VERSION "${ANDYLANG_PROJECT_VERSION}")

string(FIND "${ANDYLANG_VERSION}" "-beta" IS_BETA)
if(NOT IS_BETA EQUAL -1)
    # Removes the -beta* part
    string(SUBSTRING "${ANDYLANG_VERSION}" 0 ${IS_BETA} ANDYLANG_VERSION)
endif()

string(FIND "${ANDYLANG_VERSION}" "-alpha" IS_ALPHA)
if(NOT IS_ALPHA EQUAL -1)
    # Removes the -alpha* part
    string(SUBSTRING "${ANDYLANG_VERSION}" 0 ${IS_ALPHA} ANDYLANG_VERSION)
endif()

project(andy-lang VERSION ${ANDYLANG_VERSION})

option(BUILD_ANDY_CPP "Build andyc++" OFF)
option(BUILD_ANDY_ANALYZER "Build andy-analyzer" OFF)
option(BUILD_TESTS "Build tests" OFF)

# Get the parent directory
get_filename_component(ANDYLANG_PARENT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

include(${CMAKE_CURRENT_LIST_DIR}/modules/andy/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/modules/andy-console/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/modules/andy-file/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/modules/andy-core/CMakeLists.txt)

if(BUILD_TESTS)
    include(${CMAKE_CURRENT_LIST_DIR}/modules/andy-tests/CMakeLists.txt)
endif()

# Find all files under ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs
file(GLOB BUILTIN_LIBS "${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs/*")
# Write src/builtin_libs.cpp file
file(WRITE ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs.cpp "// This file is auto-generated. Changes will be lost.\n\n")
# Includes
file(APPEND ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs.cpp "#include <memory>\n\n")
file(APPEND ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs.cpp "#include <andy/lang/extension.hpp>\n\n")

foreach(BUILTIN_LIB ${BUILTIN_LIBS})
    # Get stem from extension from the folder
    get_filename_component(BUILTIN_LIB_NAME ${BUILTIN_LIB} NAME_WE)
    # Write extern declaration
    file(APPEND ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs.cpp "extern std::shared_ptr<andy::lang::extension> create_${BUILTIN_LIB_NAME}();\n")
endforeach()
file(APPEND ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs.cpp "\n")
file(APPEND ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs.cpp "void create_builtin_libs() {\n")
foreach(BUILTIN_LIB ${BUILTIN_LIBS})
    # Get stem from extension from the folder
    get_filename_component(BUILTIN_LIB_NAME ${BUILTIN_LIB} NAME_WE)
    # Write creation
    file(APPEND ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs.cpp "    andy::lang::extension::add_builtin(create_${BUILTIN_LIB_NAME}());\n")
    
endforeach(BUILTIN_LIB ${BUILTIN_LIBS})
file(APPEND ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs.cpp "}")

add_library(andy-lang
    ${CMAKE_CURRENT_LIST_DIR}/src/api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/method.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/preprocessor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/extension.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/class.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/interpreter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/object.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/lexer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/parser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/config.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/modules/andy-tests/src/andy-tests.cpp
    ${BUILTIN_LIBS}
    $<TARGET_OBJECTS:uva-file>
    $<TARGET_OBJECTS:uva-core>
    $<TARGET_OBJECTS:uva-console>
)

target_link_libraries(andy-lang)

add_executable(andy
    ${CMAKE_CURRENT_LIST_DIR}/src/andy.cpp
)

if(BUILD_ANDY_CPP)
    find_package(Clang REQUIRED)

    include_directories(${CLANG_INCLUDE_DIRS})

    add_executable(andyc++
        ${CMAKE_CURRENT_LIST_DIR}/src/andyc++.cpp
    )

    target_link_libraries(andyc++ PRIVATE andy-lang libclang)
endif()

if(BUILD_ANDY_ANALYZER)
    add_executable(andy-analyzer
        ${CMAKE_CURRENT_LIST_DIR}/src/analyzer.cpp
    )

    target_link_libraries(andy-analyzer PRIVATE andy-lang)
endif()

if(UNIX)
    find_package(Threads REQUIRED)

    target_link_libraries(andy-lang PRIVATE ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
    target_link_options(andy PRIVATE -Wl,--export-dynamic)

    set_target_properties(andy PROPERTIES LINK_SEARCH_START_STATIC ON)
    set_target_properties(andy PROPERTIES LINK_SEARCH_END_STATIC ON)
    target_link_options(andy PRIVATE -static-libgcc -static-libstdc++ -static)
elseif(WIN32)
    add_executable(andy-lang-installer WIN32 ${CMAKE_CURRENT_LIST_DIR}/win32/installer.cpp)
    if(EXISTS "${CMAKE_BINARY_DIR}/Release/andy.exe")
        enable_language("RC")
        set(WIN32_RESOURCES ${CMAKE_CURRENT_LIST_DIR}/win32/resources.rc)
        target_sources(andy PRIVATE ${WIN32_RESOURCES})
        target_sources(andy-lang-installer PRIVATE ${WIN32_RESOURCES})
    endif()

    add_dependencies(andy-lang-installer andy)
    SET_TARGET_PROPERTIES(andy-lang-installer PROPERTIES LINK_FLAGS "/MANIFESTUAC:/level='requireAdministrator' /uiAccess='false'")
endif()

target_link_libraries(andy PRIVATE andy-lang)

add_definitions(-DANDYLANG_PROJECT_DIR="${CMAKE_CURRENT_LIST_DIR}")
add_definitions(-DANDYLANG_BUILD_DIR="${CMAKE_BINARY_DIR}")
add_definitions(-DANDYLANG_VERSION="${ANDYLANG_PROJECT_VERSION}")

# Install andy executable
install(TARGETS andy DESTINATION bin)
if(BUILD_ANDY_ANALYZER)
    # Install andy-analyzer executable
    install(TARGETS andy-analyzer DESTINATION bin)
endif()
# Install andy-lang library
install(TARGETS andy-lang DESTINATION lib)
# Install andy-lang headers
install(DIRECTORY include/andy DESTINATION include)

install(EXPORT andy-langTargets
    FILE andy-langTargets.cmake
    NAMESPACE andy-lang::
    DESTINATION lib/cmake/andy-lang
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/andy-langConfigVersion.cmake
    VERSION ${ANDYLANG_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/andy-langConfig.cmake.in
    ${CMAKE_BINARY_DIR}/andy-langConfig.cmake
    @ONLY
)

install(FILES
    ${CMAKE_BINARY_DIR}/andy-langConfig.cmake
    ${CMAKE_BINARY_DIR}/andy-langConfigVersion.cmake
    DESTINATION lib/cmake/andy-lang
)

install(TARGETS andy-lang
    EXPORT andy-langTargets
)

# Only for debug build
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(EXTENSIONS_FOLDER "${ANDYLANG_PARENT_DIRECTORY}/andylang-extensions")

    file(GLOB EXTENSIONS "${EXTENSIONS_FOLDER}/*")

    foreach(EXTENSION ${EXTENSIONS})
        if(IS_DIRECTORY ${EXTENSION})
            # Get name from extension from the folder
            get_filename_component(EXTENSION_NAME ${EXTENSION} NAME)
            include("${EXTENSION}/CMakeLists.txt")
            target_link_libraries(andy-lang PRIVATE ${EXTENSION_NAME})
        endif()
    endforeach()
endif()

if(BUILD_TESTS)
    andy_tests_configure(andy)
    andy_tests_configure(andy-lang)
endif()