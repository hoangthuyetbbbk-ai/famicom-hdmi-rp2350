#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "dvi.h"
#include "common_dvi_pin_configs.h"

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240

static struct dvi_inst dvi0;
static uint16_t framebuf[FRAME_WIDTH * FRAME_HEIGHT];

void __not_in_flash_func(core1_main)() {
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);
    dvi_scanbuf_main_16bpp(&dvi0);
}

int main() {
    pio_set_gpio_base(DVI_DEFAULT_SERIAL_CONFIG.pio, 16);
    set_sys_clock_khz(252000, true);

    dvi0.timing = &dvi_timing_640x480p_60hz;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
    dvi0.scanbuf = framebuf;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            if (x < 106) framebuf[y*FRAME_WIDTH+x] = 0xF800;
            else if (x < 212) framebuf[y*FRAME_WIDTH+x] = 0x07E0;
            else framebuf[y*FRAME_WIDTH+x] = 0x001F;
        }
    }

    multicore_launch_core1(core1_main);
    while (true) tight_loop_contents();
}
