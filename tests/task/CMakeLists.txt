ntos_add_firmware(test_first_task
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/first_task.c
)

ntos_add_firmware(test_task_delay
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/delay.c
)
