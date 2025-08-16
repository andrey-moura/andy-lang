include(modules/andy-ui/CMakeLists.txt)

if(FOUND_DRAWING_BACKEND)
    set(ANDY_UI_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/app_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/window_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/xml_page_class.cpp
        $<TARGET_OBJECTS:andy-xml>
        $<TARGET_OBJECTS:andy-widgets>
        $<TARGET_OBJECTS:andy-drawing>
    )
else()
    set(ANDY_UI_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension_unavailable.cpp
    )
endif()