cmake_minimum_required(VERSION 3.13)

# Create executable files , also list all list source files needed used in root
add_executable(${PROJECT_NAME}
        main.c
        hub75.c
        inputs.c
        graphics.c
        SD_Card_Images.c
        flash_image_storage.c
        commands.c
)

#pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/memmap_custom.ld)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/hub75.pio)

target_sources(${PROJECT_NAME} PRIVATE main.c)

# Add libraries to target
target_link_libraries(${PROJECT_NAME} 
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_dma
        hardware_sync
        hardware_pio
        hardware_clocks
        hardware_flash
        Peripherals
        FatFs_SPI
    )

# UART options, Note USB does not work while Debugging
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Generate extra files
pico_add_extra_outputs(${PROJECT_NAME})
