include(modules/andy-net/CMakeLists.txt)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

set(ANDY_NET_EXTENSION_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/http_class.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/net_class.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/net_extension.cpp
    $<TARGET_OBJECTS:andy-net>
)