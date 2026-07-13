# Bare-metal Cortex-M3 labs.
ntos_add_firmware(lab_exception_frame
    BARE_METAL
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/01_exception_frame/main.c
)

ntos_add_firmware(lab_msp_psp
    BARE_METAL
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/02_msp_psp/main.c
)

ntos_add_firmware(lab_svc
    BARE_METAL
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/03_svc/main.c
)