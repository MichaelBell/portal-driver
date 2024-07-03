#pragma once

#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "common/pimoroni_common.hpp"
#include "common/pimoroni_bus.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"

#include <algorithm>
#include <cstring>

namespace pimoroni {

  class ST7701 : public DisplayDriver {
    spi_inst_t *spi = PIMORONI_SPI_DEFAULT_INSTANCE;

    //--------------------------------------------------
    // Variables
    //--------------------------------------------------
  private:

    // interface pins with our standard defaults where appropriate
    uint spi_cs;
    uint spi_sck;
    uint spi_dat;
    uint lcd_bl;
    uint parallel_sm;
    uint timing_sm;
    PIO st_pio;
    uint parallel_offset;
    uint timing_offset;
    uint st_dma;
    uint line_dma;

    uint d0 = 1; // First pin of 18-bit parallel interface
    uint hsync  = 19;
    uint vsync  = 20;
    uint lcd_de = 21;
    uint lcd_dot_clk = 22;

    static const uint32_t SPI_BAUD = 8'000'000;

    // Must be a power of 2
    static constexpr int NUM_LINE_BUFFERS = 4;

  public:
    // Parallel init
    ST7701(uint16_t width, uint16_t height, Rotation rotation, SPIPins control_pins, uint16_t* framebuffer,
      uint d0=1, uint hsync=19, uint vsync=20, uint lcd_de = 21, uint lcd_dot_clk = 22);

    void init();
    void cleanup() override;
    void update(PicoGraphics *graphics) override;
    void set_backlight(uint8_t brightness) override;

    void set_framebuffer(uint16_t* next_fb) {
      next_framebuffer = next_fb;
    }

    void wait_for_vsync();

    // Only to be called by ISR
    void drive_timing();
    void handle_end_of_line();
    void fill_next_line();

  private:
    void common_init();
    void configure_display(Rotation rotate);
    void write_blocking_dma(const uint8_t *src, size_t len);
    void write_blocking_parallel(const uint8_t *src, size_t len);
    void command(uint8_t command, size_t len = 0, const char *data = NULL);

    void start_line_xfer();
    void start_frame_xfer();
    void init_framebuffer();

    // Timing status
    uint16_t timing_row = 0;
    uint16_t timing_phase = 0;
    volatile bool waiting_for_vsync = false;

    uint16_t* framebuffer;
    uint16_t* next_framebuffer = nullptr;

    uint16_t* line_buffer;
    int display_row = 0;
    int row_shift = 0;
    int fill_row = 0;
  };

}
