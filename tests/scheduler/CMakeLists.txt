ntos_add_firmware(test_scheduler_cooperative
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/cooperative.c
)
# TEST-ONLY: dung NTOS_ASSERT truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(test_scheduler_cooperative PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(test_scheduler_preemptive
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/preemptive.c
)

ntos_add_firmware(test_scheduler_priority
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/priority.c
)

ntos_add_firmware(test_scheduler_ready_list
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/ready_list.c
)
# TEST-ONLY: doc ready_item.container va ready_lists[] truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(test_scheduler_ready_list PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)
