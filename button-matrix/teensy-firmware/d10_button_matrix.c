#include "d10_button_matrix.h"

static const uint8_t row_pins[D10_ROW_COUNT] = {
    D10_ROW0_PIN, D10_ROW1_PIN, D10_ROW2_PIN, D10_ROW3_PIN, D10_ROW4_PIN
};

static const uint8_t column_pins[D10_COLUMN_COUNT] = {
    D10_COL0_PIN, D10_COL1_PIN, D10_COL2_PIN, D10_COL3_PIN
};

static bool confirmed_state[D10_BUTTON_COUNT];
static bool candidate_state[D10_BUTTON_COUNT];
static uint32_t candidate_since_ms[D10_BUTTON_COUNT];

void d10_matrix_init(void)
{
    uint8_t button_id;

    for (button_id = 0; button_id < D10_BUTTON_COUNT; ++button_id) {
        confirmed_state[button_id] = false;
        candidate_state[button_id] = false;
        candidate_since_ms[button_id] = 0;
    }
}

void d10_scan_button_matrix(uint32_t now_ms, const d10_matrix_io_t *io)
{
    uint8_t row;
    uint8_t column;

    if (io == 0 || io->set_row == 0 || io->read_column == 0 || io->delay_us == 0) {
        return;
    }

    for (row = 0; row < D10_ROW_COUNT; ++row) {
        io->set_row(row_pins[row], true);
        io->delay_us(D10_ROW_SETTLE_US);

        for (column = 0; column < D10_COLUMN_COUNT; ++column) {
            const uint8_t button_id = (uint8_t)(row * D10_COLUMN_COUNT + column);
            const bool pressed = !io->read_column(column_pins[column]);

            if (pressed != candidate_state[button_id]) {
                candidate_state[button_id] = pressed;
                candidate_since_ms[button_id] = now_ms;
            } else if (pressed != confirmed_state[button_id] &&
                       (uint32_t)(now_ms - candidate_since_ms[button_id]) >= D10_DEBOUNCE_MS) {
                confirmed_state[button_id] = pressed;
                if (pressed) {
                    on_button_pressed(button_id);
                } else {
                    on_button_released(button_id);
                }
            }
        }

        io->set_row(row_pins[row], false);
    }
}

bool d10_button_is_pressed(uint8_t button_id)
{
    return button_id < D10_BUTTON_COUNT ? confirmed_state[button_id] : false;
}

const bool *d10_button_states(void)
{
    return confirmed_state;
}
