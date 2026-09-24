#pragma once
#include "hardware/pio.h"
#include "dvi_serialiser.h"

static const struct dvi_serialiser_cfg waveshare_rp2350_pizero = {
   .pio = pio0,
   .sm_tmds = {0, 1, 2},
   .pins_tmds = {36, 34, 32},
   .pins_clk = 38,
   .invert_diffpairs = false
};
#define DVI_DEFAULT_SERIAL_CONFIG waveshare_rp2350_pizero
#define DVI_DEFAULT_PIO_INST pio0
