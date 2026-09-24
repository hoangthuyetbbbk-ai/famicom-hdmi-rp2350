#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "dvi.h"
#include "dvi_serialiser.h"
#include "common_dvi_pin_configs.h"

// Cấu hình cho Waveshare RP2350-PiZero - QUAN TRỌNG
#define DVI_PIN_GROUP 16
#define DVI_PIN_TMDS0 32
#define DVI_PIN_TMDS1 34
#define DVI_PIN_TMDS2 36
#define DVI_PIN_CLK   38

static struct dvi_inst dvi0;
static struct dvi_serialiser_cfg cfg = {
    .pio = pio0,
    .sm_tmds = {0,1,2},
    .pins_tmds = {32, 34, 36},
    .pins_clk = 38,
    .invert_diffpairs = true
};

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
static uint16_t framebuf[FRAME_WIDTH * FRAME_HEIGHT];

int main() {
    // BẮT BUỘC cho PiZero vì dùng GPIO >32
    pio_set_gpio_base(pio0, 16);
    pio_set_gpio_base(pio1, 16);

    set_sys_clock_khz(252000, true);
    
    dvi0.timing = &dvi_timing_640x480p_60hz;
    dvi0.ser_cfg = cfg;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());
    
    // Tạo hình test màu
    for(int y=0; y<FRAME_HEIGHT; y++) {
        for(int x=0; x<FRAME_WIDTH; x++) {
            uint16_t color = 0;
            if(x < 106) color = 0xF800; // Đỏ
            else if(x < 212) color = 0x07E0; // Xanh lá
            else color = 0x001F; // Xanh dương
            framebuf[y*FRAME_WIDTH + x] = color;
        }
    }

    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);

    while(true) {
        // Render scale 320x240 lên 640x480
        uint32_t *tmdsbuf;
        queue_remove_blocking(&dvi0.q_tmds_free, &tmdsbuf);
        for(int y=0; y<240; y++) {
            for(int x=0; x<320; x++) {
                uint16_t c = framebuf[y*320+x];
                tmdsbuf[y*640 + x*2] = c;
                tmdsbuf[y*640 + x*2 + 1] = c;
            }
        }
        queue_add_blocking(&dvi0.q_tmds_valid, &tmdsbuf);
    }
}
