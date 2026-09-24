#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "dvi.h"
#include "dvi_serialiser.h"
#include "dvi_serialiser_cfg.h"
#include "common_dvi_pin_configs.h"

#define DVI_TIMING dvi_timing_640x480p_60hz

struct dvi_inst dvi0;

int main() {
    stdio_init_all();
    // BẮT BUỘC cho Waveshare PiZero vì GPIO HDMI > 32
    pio_set_gpio_base(DVI_DEFAULT_SERIAL_CONFIG.pio, 16);

    // Overclock cho DVI
    set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

    dvi0.timing = &DVI_TIMING;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    // Test pattern: màn hình xanh
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);

    while (true) {
        // Render loop sẽ thêm sau khi test HDMI ok
        tight_loop_contents();
    }
}
