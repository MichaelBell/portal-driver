
include(drivers/st7701_portal/st7701_portal)

set(LIB_NAME portal_driver)
add_library(${LIB_NAME} INTERFACE)

target_link_libraries(${LIB_NAME} INTERFACE st7701_portal pico_stdlib)