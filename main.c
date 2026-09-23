#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/multicore.h"
#include "dvi.h"
#include "nes_palette.h"
#include "ppu.pio.h"

#define D0 0
#define EXT0 15
#define VBL 21

static uint8_t pal_ram[32] = {0};
static uint8_t emphasis_bits = 0;
static uint16_t vram_addr = 0;
static bool addr_latch = false;
static uint8_t line_buf[256];
static struct dvi_inst dvi0;

void core0_palette() {
    PIO pio = pio0; uint sm = 0;
    uint off = pio_add_program(pio, &palette_sniffer_program);
    pio_sm_config c = palette_sniffer_program_get_default_config(off);
    sm_config_set_in_pins(&c, D0);
    sm_config_set_in_shift(&c,true,true,16);
    pio_sm_init(pio,sm,off,&c); pio_sm_set_enabled(pio,sm,true);
    while(1){
        uint32_t v = pio_sm_get_blocking(pio,sm);
        uint8_t data = v & 0xFF;
        uint8_t reg = (v >> 8) & 0x07; // $2000-$2007
        switch(reg){
            case 1: // $2001 PPUMASK
                emphasis_bits = (data >> 5) & 0x07;
                break;
            case 6: // $2006
                if(!addr_latch){
                    vram_addr = (vram_addr & 0x00FF) | ((data & 0x3F) << 8);
                    addr_latch = true;
                } else {
                    vram_addr = (vram_addr & 0xFF00) | data;
                    addr_latch = false;
                }
                break;
            case 7: // $2007
                if(vram_addr >= 0x3F00 && vram_addr <= 0x3FFF){
                    uint8_t idx = vram_addr & 0x1F;
                    if((idx & 0x13) == 0x10) idx &= ~0x10; // mirror $3F10->$3F00
                    pal_ram[idx] = data & 0x3F;
                }
                vram_addr++; 
                break;
        }
    }
}

void core1_hdmi() {
    dvi0.timing = &dvi_timing_640x480p_60hz;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());
    PIO pio = pio1; uint sm = 0;
    uint off = pio_add_program(pio, &pixel_sampler_program);
    pio_sm_config c = pixel_sampler_program_get_default_config(off);
    sm_config_set_in_pins(&c, EXT0);
    pio_sm_init(pio,sm,off,&c); pio_sm_set_enabled(pio,sm,true);

    while(1){
        while(gpio_get(VBL)==0) tight_loop_contents();
        int line_count = 0;
        for(int y=0;y<240;y++){
            for(int i=0;i<64;i++) line_buf[i] = pio_sm_get_blocking(pio,sm); // 256/4

            uint16_t *tmds = (uint16_t*)dvi_get_scanline_tmds(&dvi0);
            for(int x=0;x<64;x++) tmds[x]=0;
            for(int x=0;x<256;x++){
                uint8_t ext = line_buf[x/4] >> ((x%4)*2) & 0x0F;
                uint8_t p = pal_ram[ext & 0x0F] & 0x3F;
                uint8_t r = nes_pal[p][0];
                uint8_t g = nes_pal[p][1];
                uint8_t b = nes_pal[p][2];
                if(emphasis_bits){
                    r -= r>>2; g -= g>>2; b -= b>>2;
                }
                uint16_t rgb565 = ((r>>3)<<11)|((g>>2)<<5)|(b>>3);
                tmds[64 + x*2] = rgb565;
                tmds[64 + x*2+1] = rgb565;
            }
            for(int x=576;x<640;x++) tmds[x]=0;
            dvi_scanline_submit(&dvi0, tmds);
            line_count++;
        }
        while(line_count++ < 262){
            uint16_t *tmds = (uint16_t*)dvi_get_scanline_tmds(&dvi0);
            memset(tmds,0,640*2);
            dvi_scanline_submit(&dvi0, tmds);
        }
    }
}

int main(){
    set_sys_clock_khz(252000,true);
    for(int i=0;i<22;i++){gpio_init(i); gpio_set_dir(i,GPIO_IN);}
    multicore_launch_core1(core1_hdmi);
    core0_palette();
}
