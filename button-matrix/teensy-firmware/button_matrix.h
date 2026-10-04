#ifndef BUTTON_MATRIX_H
#define BUTTON_MATRIX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BM_ROWS 4
#define BM_COLUMNS 5
#define BM_BUTTONS (BM_ROWS * BM_COLUMNS)
#define BM_DEBOUNCE_MS 20
#define BM_SETTLE_US 50

typedef struct {
    void *context;
    void (*set_row_active)(void *context, uint8_t row, uint8_t active);
    uint8_t (*read_column)(void *context, uint8_t column);
    uint32_t (*millis)(void *context);
    void (*delay_us)(void *context, uint16_t microseconds);
    void (*button_event)(void *context, uint8_t button, uint8_t pressed);
    void (*set_led)(void *context, uint8_t button, uint16_t brightness);
} bm_io_t;

typedef struct {
    bm_io_t io;
    uint32_t changed_at[BM_BUTTONS];
    uint8_t raw[BM_BUTTONS];
    uint8_t stable[BM_BUTTONS];
} bm_t;

void bm_init(bm_t *matrix, const bm_io_t *io);
void bm_scan(bm_t *matrix);
void bm_set_led(bm_t *matrix, uint8_t button, uint16_t brightness);

#ifdef __cplusplus
}
#endif

#endif
