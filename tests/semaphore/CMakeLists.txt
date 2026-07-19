ntos_add_firmware(test_semaphore_basic
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/basic.c
)

ntos_add_firmware(test_semaphore_blocking
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/blocking.c
)
# TEST-ONLY: doc ready_lists[] truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(test_semaphore_blocking PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(test_semaphore_isr
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/semaphore_isr.c
)
