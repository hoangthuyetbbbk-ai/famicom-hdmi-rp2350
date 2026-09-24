#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "dvi.h"
#include "common_dvi_pin_configs.h"

static struct dvi_inst dvi0;

void __not_in_flash_func(core1_main)() {
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);
    while (true) {
        uint32_t *tmdsbuf;
        queue_remove_blocking(&dvi0.q_tmds_free, &tmdsbuf);
        // Test màu đỏ - bơm thẳng vào TMDS buf
        for (int i = 0; i < 320; i++) {
            tmdsbuf[i*2] = 0x0000F800; // sẽ ra sọc, chỉ để test có hình
            tmdsbuf[i*2+1] = 0x0000F800;
        }
        queue_add_blocking(&dvi0.q_tmds_valid, &tmdsbuf);
    }
}

int main() {
    pio_set_gpio_base(DVI_DEFAULT_SERIAL_CONFIG.pio, 16);
    set_sys_clock_khz(252000, true);

    dvi0.timing = &dvi_timing_640x480p_60hz;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    multicore_launch_core1(core1_main);
    while (true) tight_loop_contents();
}
