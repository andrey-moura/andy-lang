find_package(SDL3 QUIET)

if(TARGET SDL3::SDL3)
    get_target_property(SDL3_INCLUDE_DIRS SDL3::SDL3 INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT SDL3_INCLUDE_DIRS STREQUAL "SDL3_INCLUDE_DIRS-NOTFOUND")
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
else()

    set(ANDY_UI_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension_unavailable.cpp
    )

endif()
