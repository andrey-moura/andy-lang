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
    target_link_libraries(andy-lang PRIVATE ${CMAKE_DL_LIBS})
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

if(BUILD_TESTS)
    andy_tests_configure(andy)
    andy_tests_configure(andy-lang)
endif()