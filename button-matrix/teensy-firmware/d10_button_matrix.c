#include "d10_button_matrix.h"
#include "d10_can_driver.h"

static const uint8_t row_pins[BUTTON_ROWS] = {
    ROW0_PIN, ROW1_PIN, ROW2_PIN, ROW3_PIN, ROW4_PIN
};
static const uint8_t column_pins[BUTTON_COLS] = {
    COL0_PIN, COL1_PIN, COL2_PIN, COL3_PIN
};
static const uint16_t function_ids[TOTAL_BUTTONS] = {
    CAN_ID_LIDAR, CAN_ID_AUTOPILOT, CAN_ID_EMERG_COM, CAN_ID_CM5_BOOT,
    CAN_ID_MAP, CAN_ID_COM_EDIT, CAN_ID_NAV_EDIT, CAN_ID_ENGINE_MON,
    CAN_ID_TRAFFIC, CAN_ID_PFD, CAN_ID_DIRECT_TO, CAN_ID_FLIGHT_PLAN,
    CAN_ID_AIRPORT, CAN_ID_COM_SWAP, CAN_ID_NAV_SWAP, CAN_ID_XPDR,
    CAN_ID_BARO, CAN_ID_ALERT_ACK, CAN_ID_BACK, CAN_ID_MENU_DIM
};
static bool confirmed[TOTAL_BUTTONS];
static bool candidate[TOTAL_BUTTONS];
static uint32_t candidate_since[TOTAL_BUTTONS];
static uint32_t hold_start[TOTAL_BUTTONS];
static button_state_t states[TOTAL_BUTTONS];
static uint8_t current_led_brightness;

void button_matrix_init(void)
{
    for (uint8_t id = 0; id < TOTAL_BUTTONS; ++id) {
        confirmed[id] = candidate[id] = false;
        candidate_since[id] = hold_start[id] = 0;
        states[id] = BTN_STATE_IDLE;
    }
}

void scan_button_matrix(uint32_t now_ms, const d10_matrix_io_t *io)
{
    if (!io || !io->set_row || !io->read_column || !io->delay_us) {
        return;
    }
    for (uint8_t row = 0; row < BUTTON_ROWS; ++row) {
        io->set_row(row_pins[row], true);
        io->delay_us(D10_ROW_SETTLE_US);
        for (uint8_t col = 0; col < BUTTON_COLS; ++col) {
            const button_id_t id = (button_id_t)(row * BUTTON_COLS + col);
            const bool pressed = !io->read_column(column_pins[col]);
            if (states[id] == BTN_STATE_RELEASED) {
                states[id] = BTN_STATE_IDLE;
            }
            if (pressed != candidate[id]) {
                candidate[id] = pressed;
                candidate_since[id] = now_ms;
            } else if (pressed != confirmed[id] &&
                       (uint32_t)(now_ms - candidate_since[id]) >= DEBOUNCE_MS) {
                confirmed[id] = pressed;
                if (pressed) {
                    hold_start[id] = now_ms;
                    states[id] = BTN_STATE_PRESSED;
                    on_button_pressed(id);
                } else {
                    states[id] = BTN_STATE_RELEASED;
                    on_button_released(id);
                }
            }
            if (id == BTN_MENU_DIM && confirmed[id] && pressed &&
                states[id] == BTN_STATE_PRESSED &&
                (uint32_t)(now_ms - hold_start[id]) > HOLD_THRESHOLD_MS) {
                states[id] = BTN_STATE_HELD;
                on_button_held(id);
            }
        }
        io->set_row(row_pins[row], false);
    }
}

button_state_t d10_button_state(button_id_t id)
{
    return (unsigned)id < TOTAL_BUTTONS ? states[id] : BTN_STATE_IDLE;
}

static void send_event(button_id_t id, uint8_t event)
{
    if ((unsigned)id < TOTAL_BUTTONS) {
        const uint8_t data[8] = {(uint8_t)id, event, 0, 0, 0, 0, 0, 0};
        send_can_message(function_ids[id], data, sizeof(data));
    }
}

void on_button_pressed(button_id_t id)
{
    send_event(id, 1u);
}

void on_button_held(button_id_t id)
{
    if (id == BTN_MENU_DIM) {
        send_event(id, 2u);
    }
}

void on_button_released(button_id_t id)
{
    send_event(id, 0u);
}

void set_led_brightness(uint8_t brightness)
{
    current_led_brightness = brightness;
    d10_led_platform_set(brightness);
}

uint8_t d10_led_brightness(void)
{
    return current_led_brightness;
}
