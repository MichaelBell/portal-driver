#include "pico/stdlib.h"
#include <stdio.h>
#include <cstring>
#include <string>
#include <algorithm>
#include "pico/time.h"
#include "pico/platform.h"
#include <pico/multicore.h>

#include "common/pimoroni_common.hpp"
#include "drivers/st7701_portal/st7701.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"

#include "hardware/structs/ioqspi.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"

#include "drivers/plasma/apa102.hpp"

using namespace pimoroni;

static const uint BACKLIGHT = 38;

static constexpr int WIDTH = 240;
static constexpr int HEIGHT = 240;
static const uint LCD_CLK = 26;
static const uint LCD_CS = 28;
static const uint LCD_DAT = 27;
static const uint LCD_DC = PIN_UNUSED;
static const uint LCD_D0 = 1;

static uint16_t buf1[WIDTH*HEIGHT];
static uint16_t buf2[WIDTH*HEIGHT];

uint16_t* framebuffer = buf1;
uint16_t* framebuffer2 = buf2;

ST7701 st7701(
  WIDTH,
  HEIGHT,
  ROTATE_0,
  SPIPins{
    spi1,
    LCD_CS,
    LCD_CLK,
    LCD_DAT,
    PIN_UNUSED, // MISO
    LCD_DC,
    BACKLIGHT
  },
  framebuffer,
  LCD_D0
);

static const uint LED_CLK = 33;
static const uint LED_DAT = 39;

//PicoGraphics_PenRGB332 graphics(st7701.width, st7701.height, nullptr);

static void __no_inline_not_in_flash_func(set_qmi_timing)() {
    // Make sure flash is deselected - QMI doesn't appear to have a busy flag(!)
    while ((ioqspi_hw->io[1].status & IO_QSPI_GPIO_QSPI_SS_STATUS_OUTTOPAD_BITS) != IO_QSPI_GPIO_QSPI_SS_STATUS_OUTTOPAD_BITS)
        ;

    // For > 133 MHz
    qmi_hw->m[0].timing = 0x40000202;
    
    // For <= 133 MHz
    //qmi_hw->m[0].timing = 0x40000101;

    // Force a read through XIP to ensure the timing is applied
    volatile uint32_t* ptr = (volatile uint32_t*)0x14000000;
    (void) *ptr;
}

uint16_t st7701_pixel(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t value = ((r & 0xFC) << 24) | ((g & 0xFC) << 18) | ((b & 0xFC) << 12);

    // Bizarrely gcc doesn't have a reverse bits primitive
    asm volatile ( "rbit %[value], %[value]" : [value] "+r"(value));
    return value & 0xFFFF;
}

// HSV Conversion expects float inputs in the range of 0.00-1.00 for each channel
// Outputs are rgb in the range 0-255 for each channel
void from_hsv(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
  float i = floor(h * 6.0f);
  float f = h * 6.0f - i;
  v *= 255.0f;
  uint8_t p = v * (1.0f - s);
  uint8_t q = v * (1.0f - f * s);
  uint8_t t = v * (1.0f - (1.0f - f) * s);

  switch (int(i) % 6) {
    default:
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
  }
}

void core1_main() {
    st7701.init();

    while (1) __wfe();
}

int main(void) {
    set_qmi_timing();
    set_sys_clock_khz(266000, true);

    int i = 0;
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x, ++i) {
#if 0
            uint8_t r,g,b;
            from_hsv((x+y) / 480.f, 1.0f, 0.7f, r, g, b);
            framebuffer[i] = st7701_pixel(r, g, b);
#else
            if (x == y) framebuffer[i] = 0;
            else if (x == 0) framebuffer[i] = st7701_pixel(0, 255, 0);
            else if (x == 477) framebuffer[i] = st7701_pixel(255, 0, 0);
            else {
                if (y < 40) framebuffer[i] = st7701_pixel(x, x, x);
                else if (y < 80) framebuffer[i] = st7701_pixel(x, 0, 0);
                else if (y < 120) framebuffer[i] = st7701_pixel(x, x, 0);
                else if (y < 160) framebuffer[i] = st7701_pixel(0, x, 0);
                else if (y < 200) framebuffer[i] = st7701_pixel(0, x, x);
                else if (y < 240) framebuffer[i] = st7701_pixel(0, 0, x);
                else if (y < 280) framebuffer[i] = st7701_pixel(x, x >> 1, 0);
                else if (y < 320) framebuffer[i] = st7701_pixel(0, x, x >> 1);
                else if (y < 360) framebuffer[i] = st7701_pixel(x >> 1, 0, x);
                else if (y < 400) framebuffer[i] = st7701_pixel(x >> 1, x, 0);
                else if (y < 440) framebuffer[i] = st7701_pixel(0, x >> 1, x);
                else              framebuffer[i] = st7701_pixel(x, 0, x >> 1);
            }
#endif
        }
    }

    stdio_init_all();
    //while (!stdio_usb_connected());
    printf("Hello\n");
    printf("QSPI 0 timing: %08lx\n", qmi_hw->m[0].timing);

    multicore_launch_core1(core1_main);

    sleep_ms(2000);

#if 1
    uint16_t* cur_fb = framebuffer;
    uint16_t* next_fb = framebuffer2;
    memset(cur_fb, 0, WIDTH*HEIGHT*2);
    memset(next_fb, 0, WIDTH*HEIGHT*2);
    int t = 10;
    while(true) {
        i = 0;
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x, ++i) {
                uint8_t r,g,b;
                from_hsv((x+y+t) / 240.f, 1.0f, 0.7f, r, g, b);
                next_fb[i] = st7701_pixel(r, g, b);
            }
        }
        t += 10;
        st7701.set_framebuffer(next_fb);
        std::swap(next_fb, cur_fb);
        st7701.wait_for_vsync();
    }
#else
    while (1) __wfe();
#endif
}