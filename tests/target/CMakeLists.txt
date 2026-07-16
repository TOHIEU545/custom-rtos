ntos_add_firmware(test_initial_stack
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_initial_stack.c
)

ntos_add_firmware(test_first_task
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_first_task.c
)

ntos_add_firmware(test_cooperative_scheduler
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_cooperative_scheduler.c
)
# TEST-ONLY: dung NTOS_ASSERT truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(test_cooperative_scheduler PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(test_register_preservation
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_register_preservation.c
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_register_preservation_asm.S
)
# TEST-ONLY: dung NTOS_ASSERT truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(test_register_preservation PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

# Cac target test con lai se duoc them khi chang tuong ung bat dau.
