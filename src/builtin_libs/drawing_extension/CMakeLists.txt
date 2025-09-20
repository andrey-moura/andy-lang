include(modules/andy-drawing/CMakeLists.txt)

if(FOUND_DRAWING_BACKEND)
    set(ANDY_DRAWING_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/drawing_extension.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/window_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/dialog_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/page_class.cpp
        $<TARGET_OBJECTS:andy-drawing>
        $<TARGET_OBJECTS:andy-widgets>
    )
else()
    set(ANDY_DRAWING_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/drawing_extension_unavailable.cpp
    )
endif()