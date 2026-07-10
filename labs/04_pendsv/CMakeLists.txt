if(NTOS_APP STREQUAL "lab_pendsv")
    ntos_add_firmware(lab_pendsv
        MAIN ${CMAKE_CURRENT_SOURCE_DIR}/main.c
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lab_handler.S
    )
endif()
