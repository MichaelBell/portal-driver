set(OUTPUT_NAME st7701_test)
add_executable(${OUTPUT_NAME} st7701_test_240.cpp)

include(common/pimoroni_i2c)
include(common/pimoroni_bus)
include(libraries/bitmap_fonts/bitmap_fonts)
include(libraries/hershey_fonts/hershey_fonts)
include(libraries/pico_graphics/pico_graphics)
include(drivers/plasma/plasma)

target_link_libraries(${OUTPUT_NAME}
        hardware_spi
#        pico_graphics
        pico_multicore
        st7701_portal
        plasma
#        button
)

# enable usb output
pico_enable_stdio_usb(${OUTPUT_NAME} 1)

pico_add_extra_outputs(${OUTPUT_NAME})
