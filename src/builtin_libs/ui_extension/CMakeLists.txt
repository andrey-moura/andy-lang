find_package(SDL3 QUIET)

if(SDL3_FOUND)
    add_compile_definitions(
        ANDY_USE_SDL3
    )

    include(modules/andy/include/uva/xml/CMakeLists.txt)
    include(modules/andy-ui/CMakeLists.txt)
    include(modules/andy-widgets/CMakeLists.txt)

    set(ANDY_UI_EXTENSION_SOURCES   
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/app_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/frame_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/dialog_class.cpp
        $<TARGET_OBJECTS:uva-xml>
        $<TARGET_OBJECTS:andy-ui>
        $<TARGET_OBJECTS:uva-widgets>
    )
else()

    set(ANDY_UI_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension_unavailable.cpp
    )

endif()
