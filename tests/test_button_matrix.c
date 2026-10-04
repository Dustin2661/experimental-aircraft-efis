#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../button-matrix/teensy-firmware/d10_button_matrix.h"

static uint8_t row_activations;
static uint8_t last_active_row;
static uint8_t active_row;
static uint8_t presses;
static uint8_t releases;
static int last_event_button;
static bool switch_2_pressed;
static bool switch_20_pressed;

static void set_row(uint8_t pin, bool active)
{
    if (active) {
        last_active_row = pin;
        active_row = pin;
        ++row_activations;
    }
}

static bool read_column(uint8_t pin)
{
    const bool sw2 = switch_2_pressed &&
                     active_row == D10_ROW0_PIN &&
                     pin == D10_COL1_PIN;
    const bool sw20 = switch_20_pressed &&
                      active_row == D10_ROW4_PIN &&
                      pin == D10_COL3_PIN;
    return !(sw2 || sw20);
}

static void delay_us(uint32_t microseconds)
{
    assert(microseconds == D10_ROW_SETTLE_US);
}

void on_button_pressed(int button_id)
{
    ++presses;
    last_event_button = button_id;
}

void on_button_released(int button_id)
{
    ++releases;
    last_event_button = button_id;
}

int main(void)
{
    const d10_matrix_io_t io = {set_row, read_column, delay_us};
    d10_matrix_init();

    d10_scan_button_matrix(100u, &io);
    assert(row_activations == D10_ROW_COUNT);
    assert(last_active_row == D10_ROW4_PIN);
    assert(presses == 0u);

    switch_2_pressed = true;
    d10_scan_button_matrix(110u, &io);
    assert(presses == 0u);
    d10_scan_button_matrix(129u, &io);
    assert(presses == 0u);
    d10_scan_button_matrix(130u, &io);
    assert(presses == 1u);
    assert(last_event_button == D10_SW2);
    assert(d10_button_is_pressed(D10_SW2));

    switch_2_pressed = false;
    d10_scan_button_matrix(140u, &io);
    d10_scan_button_matrix(160u, &io);
    assert(releases == 1u);
    assert(last_event_button == D10_SW2);
    assert(!d10_button_is_pressed(D10_SW2));

    switch_20_pressed = true;
    d10_scan_button_matrix(UINT32_MAX - 10u, &io);
    assert(presses == 1u);
    d10_scan_button_matrix(10u, &io);
    assert(presses == 2u);
    assert(last_event_button == D10_SW20);
    assert(d10_button_is_pressed(D10_SW20));

    puts("matrix scan/debounce tests passed");
    return 0;
}
