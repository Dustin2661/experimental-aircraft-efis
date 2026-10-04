#include "button_matrix.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    int8_t active_row;
    uint8_t pressed[BM_BUTTONS];
    uint32_t now;
    uint8_t event_count;
    uint8_t last_button;
    uint8_t last_pressed;
    uint8_t led_count;
    uint8_t last_led;
    uint16_t last_brightness;
} test_context_t;

static void set_row(void *context, uint8_t row, uint8_t active)
{
    test_context_t *test = context;
    test->active_row = active ? (int8_t)row : -1;
}

static uint8_t read_column(void *context, uint8_t column)
{
    test_context_t *test = context;
    if (test->active_row < 0) {
        return 1;
    }
    return (uint8_t)!test->pressed[test->active_row * BM_COLUMNS + column];
}

static uint32_t get_millis(void *context)
{
    return ((test_context_t *)context)->now;
}

static void delay_us(void *context, uint16_t delay)
{
    (void)context;
    (void)delay;
}

static void button_event(void *context, uint8_t button, uint8_t pressed)
{
    test_context_t *test = context;
    ++test->event_count;
    test->last_button = button;
    test->last_pressed = pressed;
}

static void set_led(void *context, uint8_t button, uint16_t brightness)
{
    test_context_t *test = context;
    ++test->led_count;
    test->last_led = button;
    test->last_brightness = brightness;
}

int main(void)
{
    test_context_t test;
    bm_t matrix;
    bm_io_t io;

    memset(&test, 0, sizeof(test));
    test.active_row = -1;
    io.context = &test;
    io.set_row_active = set_row;
    io.read_column = read_column;
    io.millis = get_millis;
    io.delay_us = delay_us;
    io.button_event = button_event;
    io.set_led = set_led;
    bm_init(&matrix, &io);

    test.pressed[0] = 1;
    bm_scan(&matrix);
    assert(test.event_count == 0);
    test.now = 19;
    bm_scan(&matrix);
    assert(test.event_count == 0);
    test.now = 20;
    bm_scan(&matrix);
    assert(test.event_count == 1);
    assert(test.last_button == 0 && test.last_pressed == 1);

    test.pressed[0] = 0;
    bm_scan(&matrix);
    test.now = 39;
    bm_scan(&matrix);
    assert(test.event_count == 1);
    test.now = 40;
    bm_scan(&matrix);
    assert(test.event_count == 2);
    assert(test.last_button == 0 && test.last_pressed == 0);

    test.now = UINT32_MAX - 10U;
    test.pressed[19] = 1;
    bm_scan(&matrix);
    test.now = 8;
    bm_scan(&matrix);
    assert(test.event_count == 2);
    test.now = 9;
    bm_scan(&matrix);
    assert(test.event_count == 3);
    assert(test.last_button == 19 && test.last_pressed == 1);

    bm_set_led(&matrix, 4, 2048);
    assert(test.led_count == 1 && test.last_led == 4 && test.last_brightness == 2048);
    bm_set_led(&matrix, 20, 100);
    bm_set_led(&matrix, 0, 5000);
    assert(test.led_count == 2 && test.last_led == 0 && test.last_brightness == 4095);
    return 0;
}
