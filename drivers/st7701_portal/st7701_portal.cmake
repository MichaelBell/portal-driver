set(DRIVER_NAME st7701_portal)
add_library(${DRIVER_NAME} INTERFACE)

target_sources(${DRIVER_NAME} INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/st7701.cpp)

pico_generate_pio_header(${DRIVER_NAME} ${CMAKE_CURRENT_LIST_DIR}/st7701_parallel.pio)
pico_generate_pio_header(${DRIVER_NAME} ${CMAKE_CURRENT_LIST_DIR}/st7701_timing.pio)

target_include_directories(${DRIVER_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})

# Pull in pico libraries that we need
target_link_libraries(${DRIVER_NAME} INTERFACE pico_stdlib pimoroni_bus hardware_spi hardware_pwm hardware_pio hardware_dma pico_graphics)
