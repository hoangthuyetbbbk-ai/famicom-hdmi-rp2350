#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "dvi.h"
#include "dvi_serialiser.h"

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240

static struct dvi_inst dvi0;
static uint16_t framebuf[FRAME_WIDTH * FRAME_HEIGHT];

static const struct dvi_serialiser_cfg pizero_cfg = {
   .pio = pio0,
   .sm_tmds = {0, 1, 2},
   .pins_tmds = {36, 34, 32},
   .pins_clk = 38,
   .invert_diffpairs = false
};

int main() {
    // BẮT BUỘC cho PiZero
    pio_set_gpio_base(pio0, 16);
    pio_set_gpio_base(pio1, 16);

    set_sys_clock_khz(252000, true);

    dvi0.timing = &dvi_timing_640x480p_60hz;
    dvi0.ser_cfg = pizero_cfg;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    // Tạo sọc màu test
    for(int y=0; y<FRAME_HEIGHT; y++){
        for(int x=0; x<FRAME_WIDTH; x++){
            uint16_t c = 0;
            if(x < 106) c = 0xF800;
            else if(x < 212) c = 0x07E0;
            else c = 0x001F;
            framebuf[y*FRAME_WIDTH+x] = c;
        }
    }

    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);

    while(true){
        uint32_t *tmdsbuf;
        queue_remove_blocking(&dvi0.q_tmds_free, &tmdsbuf);
        for(uint y=0; y<FRAME_HEIGHT; y++){
            uint16_t *line = &framebuf[y*FRAME_WIDTH];
            for(uint x=0; x<FRAME_WIDTH; x++){
                // scale 2x
                tmdsbuf[(y*2)*640 + x*2] = line[x];
                tmdsbuf[(y*2)*640 + x*2+1] = line[x];
                tmdsbuf[(y*2+1)*640 + x*2] = line[x];
                tmdsbuf[(y*2+1)*640 + x*2+1] = line[x];
            }
        }
        queue_add_blocking(&dvi0.q_tmds_valid, &tmdsbuf);
    }
}
