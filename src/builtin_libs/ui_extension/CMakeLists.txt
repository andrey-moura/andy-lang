if()
    set(ANDY_UI_EXTENSION_SOURCES   
        ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs/ui_extension.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs/ui_extension/app_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs/ui_extension/frame_class.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/builtin_libs/ui_extension/dialog_class.cpp
    )
else()

    set(ANDY_UI_EXTENSION_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/ui_extension_unavailable.cpp
    )

    return()

endif()
