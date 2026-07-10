if(NTOS_APP STREQUAL "lab_exception_frame")
    ntos_add_firmware(lab_exception_frame
        MAIN ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    )
endif()
