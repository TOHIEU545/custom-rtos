ntos_add_firmware(test_semaphore
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/semaphore.c
)
# TEST-ONLY: doc ready_lists[]/ntos_list_is_empty() truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(test_semaphore PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(test_semaphore_isr
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/semaphore_isr.c
)
