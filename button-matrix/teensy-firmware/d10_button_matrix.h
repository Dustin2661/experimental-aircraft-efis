#ifndef D10_BUTTON_MATRIX_H
#define D10_BUTTON_MATRIX_H

#include <stdbool.h>
#include <stdint.h>

#define D10_ROW_COUNT 5u
#define D10_COLUMN_COUNT 4u
#define D10_BUTTON_COUNT (D10_ROW_COUNT * D10_COLUMN_COUNT)
#define D10_SCAN_INTERVAL_MS 10u
#define D10_DEBOUNCE_MS 20u
#define D10_ROW_SETTLE_US 50u

#define D10_ROW0_PIN 0u
#define D10_ROW1_PIN 1u
#define D10_ROW2_PIN 2u
#define D10_ROW3_PIN 3u
#define D10_ROW4_PIN 4u

#define D10_COL0_PIN 5u
#define D10_COL1_PIN 6u
#define D10_COL2_PIN 7u
#define D10_COL3_PIN 8u

#define D10_LED_PWM_PIN 9u

typedef enum {
    D10_SW1 = 0, D10_SW2, D10_SW3, D10_SW4,
    D10_SW5, D10_SW6, D10_SW7, D10_SW8,
    D10_SW9, D10_SW10, D10_SW11, D10_SW12,
    D10_SW13, D10_SW14, D10_SW15, D10_SW16,
    D10_SW17, D10_SW18, D10_SW19, D10_SW20
} d10_button_id_t;

typedef struct {
    void (*set_row)(uint8_t pin, bool active);
    bool (*read_column)(uint8_t pin);
    void (*delay_us)(uint32_t microseconds);
} d10_matrix_io_t;

void d10_matrix_init(void);
void d10_scan_button_matrix(uint32_t now_ms, const d10_matrix_io_t *io);
bool d10_button_is_pressed(uint8_t button_id);
const bool *d10_button_states(void);

void on_button_pressed(int button_id);
void on_button_released(int button_id);
void set_led_brightness(uint8_t brightness);

#endif
