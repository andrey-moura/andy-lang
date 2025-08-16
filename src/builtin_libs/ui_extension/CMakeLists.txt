include(modules/andy-ui/CMakeLists.txt)

if(FOUND_DRAWING_BACKEND)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
    set(ANDY_UI_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/app_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension.cpp
        $<TARGET_OBJECTS:andy-xml>
        $<TARGET_OBJECTS:andy-ui>
    )
else()
    set(ANDY_UI_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension_unavailable.cpp
    )
endif()