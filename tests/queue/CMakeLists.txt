ntos_add_firmware(test_queue_fifo
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/fifo.c
)

ntos_add_firmware(test_queue_blocking_receive
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/blocking_receive.c
)
# TEST-ONLY: doc ready_lists[] truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(test_queue_blocking_receive PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(test_queue_blocking_send
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/blocking_send.c
)
# TEST-ONLY: doc ready_lists[] truc tiep (xem NTOS_CODING_CONVENTION.md muc 2.4).
target_include_directories(test_queue_blocking_send PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)

ntos_add_firmware(test_queue_isr
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/queue_isr.c
)
