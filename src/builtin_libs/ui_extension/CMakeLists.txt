find_package(SDL3 QUIET)

if(SDL3_FOUND)
    include(${CMAKE_CURRENT_LIST_DIR}/modules/andy/include/uva/xml/CMakeLists.txt)
    include(${CMAKE_CURRENT_LIST_DIR}/modules/andy-ui/CMakeLists.txt)
    include(${CMAKE_CURRENT_LIST_DIR}/modules/andy-widgets/CMakeLists.txt)

    set(ANDY_UI_EXTENSION_SOURCES   
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/app_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/frame_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/dialog_class.cpp
        $<TARGET_OBJECTS:uva-xml>
        $<TARGET_OBJECTS:andy-ui>
    )
else()

    set(ANDY_UI_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension_unavailable.cpp
    )

endif()
