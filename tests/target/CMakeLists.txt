ntos_add_firmware(chang02_initial_stack
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang02_initial_stack.c
)

ntos_add_firmware(chang03_first_task
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang03_first_task.c
)

ntos_add_firmware(chang04_cooperative_scheduler
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang04_cooperative_scheduler.c
)
# TEST-ONLY: dung NTOS_ASSERT truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(chang04_cooperative_scheduler PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(chang05_preemption
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang05_preemption.c
)

ntos_add_firmware(chang06_priority_scheduler
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang06_priority_scheduler.c
)

ntos_add_firmware(chang07_delay
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang07_delay.c
)

ntos_add_firmware(chang08_ready_list_scheduler
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang08_ready_list_scheduler.c
)
# TEST-ONLY: doc ready_item.container va ready_lists[] truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(chang08_ready_list_scheduler PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(chang09_semaphore
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang09_semaphore.c
)
# TEST-ONLY: doc ready_lists[]/ntos_list_is_empty() truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(chang09_semaphore PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(chang09_semaphore_isr
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang09_semaphore_isr.c
)

ntos_add_firmware(chang09_queue
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang09_queue.c
)
# TEST-ONLY: doc ready_lists[]/ntos_list_is_empty() truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(chang09_queue PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(chang09_queue_isr
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/chang09_queue_isr.c
)

# Cac target test con lai se duoc them khi chang tuong ung bat dau.
