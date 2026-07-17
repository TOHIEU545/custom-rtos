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

ntos_add_firmware(test_preemption
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_preemption.c
)

ntos_add_firmware(test_task_states
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_task_states.c
)
# TEST-ONLY: dung ntos_current_tcb va dat state truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(test_task_states PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(test_priority_scheduler
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_priority_scheduler.c
)

ntos_add_firmware(test_idle_task
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/test_idle_task.c
)

# Cac target test con lai se duoc them khi chang tuong ung bat dau.
