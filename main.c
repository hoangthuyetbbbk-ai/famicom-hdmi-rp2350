#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "dvi.h"
#include "common_dvi_pin_configs.h"
#define W 320
#define H 240
static struct dvi_inst dvi0;
static uint16_t fb[H][W];
static uint line=0;
void __not_in_flash_func(scan_cb)(){
    uint16_t *p=NULL;
    while(queue_try_remove_u32(&dvi0.q_colour_free,(uint32_t*)&p));
    p=&fb[line][0];
    queue_add_blocking_u32(&dvi0.q_colour_valid,(uint32_t*)&p);
    if(++line>=H) line=0;
}
void __not_in_flash_func(core1_main)(){
    dvi_register_irqs_this_core(&dvi0,DMA_IRQ_0);
    dvi_start(&dvi0);
    dvi_scanbuf_main_16bpp(&dvi0);
}
int main(){
    pio_set_gpio_base(DVI_DEFAULT_SERIAL_CONFIG.pio,16);
    set_sys_clock_khz(252000,true);
    dvi0.timing=&dvi_timing_640x480p_60hz;
    dvi0.ser_cfg=DVI_DEFAULT_SERIAL_CONFIG;
    dvi0.scanline_callback=scan_cb;
    dvi_init(&dvi0,next_striped_spin_lock_num(),next_striped_spin_lock_num());
    for(int y=0;y<H;y++) for(int x=0;x<W;x++){
        if(x<106) fb[y][x]=0xF800; else if(x<212) fb[y][x]=0x07E0; else fb[y][x]=0x001F;
    }
    multicore_launch_core1(core1_main);
    while(1) tight_loop_contents();
}
