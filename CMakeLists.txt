#Require a minimum version
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" ANDYLANG_PROJECT_VERSION)
string(STRIP "${ANDYLANG_PROJECT_VERSION}" ANDYLANG_PROJECT_VERSION)

project(andy-lang VERSION ${ANDYLANG_PROJECT_VERSION})

option(BUILD_ANDY_CPP "Build andyc++" OFF)
option(CLONE_AND_INCLUDE_DEPENDENCIES "Clone and include dependencies" OFF)

# Get the parent directory
get_filename_component(ANDYLANG_PARENT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)

if(CLONE_AND_INCLUDE_DEPENDENCIES)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/dependencies)
    set(GIT_OUTPUT ${CMAKE_CURRENT_LIST_DIR}/dependencies)

    set(ANDYLANG_DEPENDENCIES "andy;andy-core;andy-file;andy-console")

    foreach(ANDYLANG_DEPENDENCY ${ANDYLANG_DEPENDENCIES})
        if(NOT EXISTS ${GIT_OUTPUT}/${ANDYLANG_DEPENDENCY})
            execute_process(
                COMMAND git clone https://www.github.com/andrey-moura/${ANDYLANG_DEPENDENCY} ${GIT_OUTPUT}/${ANDYLANG_DEPENDENCY} --recursive
                WORKING_DIRECTORY ${GIT_OUTPUT}
            )
        endif()
        MESSAGE(STATUS "Adding dependency ${GIT_OUTPUT}/${ANDYLANG_DEPENDENCY}")
        include(${GIT_OUTPUT}/${ANDYLANG_DEPENDENCY}/CMakeLists.txt)
    endforeach()
else()
    find_package(andy REQUIRED)
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(include/fmt/include)

add_library(andy-lang
    ${CMAKE_CURRENT_LIST_DIR}/src/method.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/preprocessor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/extension.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/class.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/interpreter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/object.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/lexer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/parser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/config.cpp
)

target_link_libraries(andy-lang uva-file uva-core uva-console)

add_executable(andy
    ${CMAKE_CURRENT_LIST_DIR}/src/andy.cpp
)

if(BUILD_ANDY_CPP)
    find_package(Clang REQUIRED)

    include_directories(${CLANG_INCLUDE_DIRS})

    add_executable(andyc++
        ${CMAKE_CURRENT_LIST_DIR}/src/andyc++.cpp
    )

    target_link_libraries(andyc++ uva-file uva-console andy-lang libclang)
endif()

if(UNIX)
    target_link_libraries(andy-lang ${CMAKE_DL_LIBS})
    target_link_options(andy PRIVATE -Wl,--export-dynamic)
endif()

target_link_libraries(andy PRIVATE andy-lang)

add_definitions(-DANDYLANG_PROJECT_DIR="${CMAKE_CURRENT_LIST_DIR}")
add_definitions(-DANDYLANG_BUILD_DIR="${CMAKE_BINARY_DIR}")
add_definitions(-DANDYLANG_VERSION="${ANDYLANG_PROJECT_VERSION}")

# Install andy executable
install(TARGETS andy DESTINATION bin)
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
        target_link_libraries(andy-lang ${EXTENSION_NAME})
    endif()
endforeach()
