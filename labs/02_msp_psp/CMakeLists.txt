if(NTOS_APP STREQUAL "lab_msp_psp")
    ntos_add_firmware(lab_msp_psp
        MAIN ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    )
endif()
