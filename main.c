#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "dvi.h"
#include "common_dvi_pin_configs.h"
#include "dvi_serialiser.h"

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240

static struct dvi_inst dvi0;
static uint16_t framebuf[FRAME_WIDTH * FRAME_HEIGHT];

void core1_main() {
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);
    dvi_scanbuf_main_16bpp(&dvi0);
    __builtin_unreachable();
}

int main() {
    // === BẮT BUỘC CHO WAVESHARE RP2350-PIZERO ===
    pio_set_gpio_base(DVI_DEFAULT_SERIAL_CONFIG.pio, 16);

    setup_default_uart();
    printf("Famicom HDMI RP2350-PiZero starting...\n");

    // 252MHz = DVI clock chuẩn
    set_sys_clock_khz(252000, true);

    // Cấu hình DVI
    dvi0.timing = &dvi_timing_640x480p_60hz;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    // Tạo hình test 3 sọc màu
    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            uint16_t color;
            if (x < 106) color = 0xF800;       // Đỏ
            else if (x < 212) color = 0x07E0;  // Xanh lá
            else color = 0x001F;               // Xanh dương
            framebuf[y * FRAME_WIDTH + x] = color;
        }
    }

    // Framebuffer 16bpp, scale 2x lên 640x480
    dvi_get_blank_settings(&dvi0)->top = 4 * 2;
    dvi_get_blank_settings(&dvi0)->bottom = 4 * 2;
    dvi0.scanline_callback = NULL;
    
    uint16_t *fb = framebuf;
    dvi0.scanline_callback = NULL;

    // Chạy core1 cho DVI
    void (*func)(void) = core1_main;
    multicore_launch_core1(func);

    // Core0 cung cấp framebuffer liên tục
    while (true) {
        uint32_t *buf;
        queue_remove_blocking(&dvi0.q_tmds_free, &buf);
        // DVI cần 640 pixel mỗi dòng, ta nhân đôi 320 -> 640
        for (int y = 0; y < FRAME_HEIGHT; y++) {
            for (int x = 0; x < FRAME_WIDTH; x++) {
                buf[y * 2 * 640 + x * 2] = framebuf[y * FRAME_WIDTH + x];
                buf[y * 2 * 640 + x * 2 + 1] = framebuf[y * FRAME_WIDTH + x];
                buf[(y * 2 + 1) * 640 + x * 2] = framebuf[y * FRAME_WIDTH + x];
                buf[(y * 2 + 1) * 640 + x * 2 + 1] = framebuf[y * FRAME_WIDTH + x];
            }
        }
        // Lặp lại cho 480 dòng (vblank đã tính)
        queue_add_blocking(&dvi0.q_tmds_valid, &buf);
    }
}
