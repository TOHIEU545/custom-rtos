if(NTOS_APP STREQUAL "example_full_demo")
    ntos_add_firmware(example_full_demo MAIN ${CMAKE_CURRENT_SOURCE_DIR}/main.c CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()
