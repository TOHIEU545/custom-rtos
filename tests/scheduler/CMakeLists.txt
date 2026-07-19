ntos_add_firmware(chang04_cooperative_scheduler
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/cooperative.c
)
# TEST-ONLY: dung NTOS_ASSERT truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(chang04_cooperative_scheduler PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(chang05_preemption
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/preemptive.c
)

ntos_add_firmware(chang06_priority_scheduler
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/priority.c
)

ntos_add_firmware(chang08_ready_list_scheduler
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/ready_list.c
)
# TEST-ONLY: doc ready_item.container va ready_lists[] truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(chang08_ready_list_scheduler PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)
