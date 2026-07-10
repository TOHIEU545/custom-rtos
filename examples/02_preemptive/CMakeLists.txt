if(NTOS_APP STREQUAL "example_preemptive")
    ntos_add_firmware(example_preemptive MAIN ${CMAKE_CURRENT_SOURCE_DIR}/main.c CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()
