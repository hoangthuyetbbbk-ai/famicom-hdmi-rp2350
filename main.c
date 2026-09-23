#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "hardware/structs/bus_ctrl.h"
#include "dvi.h"
#include "dvi_serialiser_cfg.h"
#include "nes_palette.h"
#include "ppu.pio.h"

#define D0 0
#define EXT0 15
#define VBL 21

static uint8_t pal_ram[32] = {0};
static uint8_t emphasis_bits = 0;
static uint16_t vram_addr = 0;
static bool addr_latch = false;
static uint8_t line_buf[64];
static struct dvi_inst dvi0;

// Cấu hình HDMI riêng cho Waveshare RP2350 PiZero GPIO 32-39
static const struct dvi_serialiser_cfg waveshare_rp2350_pizero_cfg = {
   .pio = DVI_DEFAULT_PIO_INST,
   .sm_tmds = {0, 1, 2},
   .pins_tmds = {36, 34, 32},
   .pins_clk = 38,
   .invert_diffpairs = false
};

void core0_palette() {
    PIO pio = pio0;
    uint sm = 0;
    uint off = pio_add_program(pio, &palette_sniffer_program);
    pio_sm_config c = palette_sniffer_program_get_default_config(off);
    sm_config_set_in_pins(&c, D0);
    sm_config_set_in_shift(&c, true, true, 16);
    pio_sm_init(pio, sm, off, &c);
    pio_sm_set_enabled(pio, sm, true);
    while(1){
        uint32_t v = pio_sm_get_blocking(pio, sm);
        uint8_t data = v & 0xFF;
        uint8_t reg = (v >> 8) & 0x07;
        switch(reg){
            case 1: emphasis_bits = (data >> 5) & 0x07; break;
            case 6:
                if(!addr_latch){ vram_addr = (vram_addr & 0x00FF) | ((data & 0x3F) << 8); addr_latch = true; }
                else { vram_addr = (vram_addr & 0xFF00) | data; addr_latch = false; }
                break;
            case 7:
                if(vram_addr >= 0x3F00){ uint8_t idx = vram_addr & 0x1F; if((idx & 0x13) == 0x10) idx &= ~0x10; pal_ram[idx] = data & 0x3F; }
                vram_addr++; break;
        }
    }
}

void core1_hdmi() {
    dvi0.timing = &dvi_timing_640x480p_60hz;
    dvi0.ser_cfg = waveshare_rp2350_pizero_cfg;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());
    pio_set_gpio_base(dvi0.ser_cfg.pio, 16);
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);

    PIO pio = pio1;
    uint sm = 0;
    uint off = pio_add_program(pio, &pixel_sampler_program);
    pio_sm_config c = pixel_sampler_program_get_default_config(off);
    sm_config_set_in_pins(&c, EXT0);
    sm_config_set_in_shift(&c, true, true, 32);
    pio_sm_init(pio, sm, off, &c);
    pio_sm_set_enabled(pio, sm, true);

    while(1){
        while(gpio_get(VBL)==0) tight_loop_contents();
        for(int y=0;y<240;y++){
            for(int i=0;i<64;i++) line_buf[i] = pio_sm_get_blocking(pio, sm) & 0xFF;

            uint16_t *buf = dvi_get_line_buffer(&dvi0);
            for(int x=0;x<64;x++) buf[x]=0;
            for(int x=0;x<256;x++){
                uint8_t ext = (line_buf[x/4] >> ((x%4)*2)) & 0x03;
                uint8_t p = pal_ram[ext & 0x0F] & 0x3F;
                uint8_t r = nes_pal[p][0];
                uint8_t g = nes_pal[p][1];
                uint8_t b = nes_pal[p][2];
                if(emphasis_bits){ r -= r>>2; g -= g>>2; b -= b>>2; }
                uint16_t rgb565 = ((r>>3)<<11)|((g>>2)<<5)|(b>>3);
                buf[64 + x*2] = rgb565;
                buf[64 + x*2+1] = rgb565;
            }
            for(int x=576;x<640;x++) buf[x]=0;
            queue_add_blocking(&dvi0.q_colour_valid, &buf);
            queue_remove_blocking(&dvi0.q_colour_free, &buf);
        }
        for(int y=240;y<262;y++){
            uint16_t *buf = dvi_get_line_buffer(&dvi0);
            memset(buf, 0, 640*2);
            queue_add_blocking(&dvi0.q_colour_valid, &buf);
            queue_remove_blocking(&dvi0.q_colour_free, &buf);
        }
    }
}

int main(){
    set_sys_clock_khz(252000,true);
    for(int i=0;i<22;i++){ gpio_init(i); gpio_set_dir(i,GPIO_IN); gpio_set_pulls(i,false,false); }
    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_PROC1_BITS;
    multicore_launch_core1(core1_hdmi);
    core0_palette();
}