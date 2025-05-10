if(WIN32 OR UNIX)
    set(ANDY_UI_EXTENSION_SOURCES   
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/app_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/frame_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/dialog_class.cpp
    )
else()

    set(ANDY_UI_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension_unavailable.cpp
    )

    return()

endif()
