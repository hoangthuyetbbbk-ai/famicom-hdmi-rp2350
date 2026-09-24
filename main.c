#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "lib/libdvi/dvi.h"
#include "lib/libdvi/common_dvi_pin_configs.h"

#define W 320
#define H 240
static struct dvi_inst dvi0;
static uint16_t fb[W*H];

void __not_in_flash_func(core1_main)(){
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);
    // Hàm này có trong lib xịn, nó quét fb liên tục, không bao giờ đen
    dvi_static_framebuf_main_16bpp(&dvi0, fb, W, H);
}

int main(){
    pio_set_gpio_base(DVI_DEFAULT_PIO_INST, 16);
    set_sys_clock_khz(252000, true);

    dvi0.timing = &dvi_timing_640x480p_60hz;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    for(int y=0;y<H;y++) for(int x=0;x<W;x++){
        if(x<106) fb[y*W+x]=0xF800;
        else if(x<212) fb[y*W+x]=0x07E0;
        else fb[y*W+x]=0x001F;
    }

    multicore_launch_core1(core1_main);
    while(1) tight_loop_contents();
}
