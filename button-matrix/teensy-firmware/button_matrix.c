#include "button_matrix.h"

#include <stddef.h>

void bm_init(bm_t *matrix, const bm_io_t *io)
{
    uint8_t button;
    const uint32_t now = io->millis(io->context);

    matrix->io = *io;
    for (button = 0; button < BM_BUTTONS; ++button) {
        matrix->raw[button] = 0;
        matrix->stable[button] = 0;
        matrix->changed_at[button] = now;
    }
    for (button = 0; button < BM_ROWS; ++button) {
        matrix->io.set_row_active(matrix->io.context, button, 0);
    }
}

void bm_scan(bm_t *matrix)
{
    uint8_t row;
    uint8_t column;
    uint32_t now;

    for (row = 0; row < BM_ROWS; ++row) {
        matrix->io.set_row_active(matrix->io.context, row, 1);
        matrix->io.delay_us(matrix->io.context, BM_SETTLE_US);
        for (column = 0; column < BM_COLUMNS; ++column) {
            const uint8_t button = (uint8_t)(row * BM_COLUMNS + column);
            const uint8_t pressed =
                (uint8_t)(matrix->io.read_column(matrix->io.context, column) == 0);
            if (pressed != matrix->raw[button]) {
                matrix->raw[button] = pressed;
                matrix->changed_at[button] = matrix->io.millis(matrix->io.context);
            }
        }
        matrix->io.set_row_active(matrix->io.context, row, 0);
    }

    now = matrix->io.millis(matrix->io.context);
    for (row = 0; row < BM_BUTTONS; ++row) {
        if ((matrix->raw[row] != matrix->stable[row]) &&
            ((uint32_t)(now - matrix->changed_at[row]) >= BM_DEBOUNCE_MS)) {
            matrix->stable[row] = matrix->raw[row];
            if (matrix->io.button_event != NULL) {
                matrix->io.button_event(matrix->io.context, row, matrix->stable[row]);
            }
        }
    }
}

void bm_set_led(bm_t *matrix, uint8_t button, uint16_t brightness)
{
    if ((button < BM_BUTTONS) && (matrix->io.set_led != NULL)) {
        if (brightness > 4095) {
            brightness = 4095;
        }
        matrix->io.set_led(matrix->io.context, button, brightness);
    }
}
