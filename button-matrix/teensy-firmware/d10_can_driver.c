#include "d10_button_matrix.h"
#include "d10_can_driver.h"

static uint32_t last_receive_ms;
static uint32_t tx_errors;
static bool can_initialized;

void d10_can_driver_init(uint32_t now_ms)
{
    last_receive_ms = now_ms;
    tx_errors = 0;
    can_initialized = d10_can_platform_begin(D10_CAN_BAUD_RATE);
}

static void send_button_event(uint8_t button_id, uint8_t pressed)
{
    uint8_t data[D10_CAN_FRAME_LENGTH] = {0};

    if (button_id >= D10_BUTTON_COUNT) {
        return;
    }

    data[0] = button_id;
    data[1] = pressed;
    if (!can_initialized ||
        !d10_can_platform_send(D10_CAN_BUTTON_EVENT_ID, data, D10_CAN_FRAME_LENGTH)) {
        ++tx_errors;
    }
    d10_can_platform_log(false, D10_CAN_BUTTON_EVENT_ID, data, D10_CAN_FRAME_LENGTH);
}

void on_button_pressed(int button_id)
{
    if (button_id >= 0) {
        send_button_event((uint8_t)button_id, 1u);
    }
}

void on_button_released(int button_id)
{
    if (button_id >= 0) {
        send_button_event((uint8_t)button_id, 0u);
    }
}

void set_led_brightness(uint8_t brightness)
{
    d10_led_platform_set(brightness);
}

void d10_can_service(uint32_t now_ms)
{
    uint32_t can_id;
    uint8_t data[D10_CAN_FRAME_LENGTH];
    uint8_t length;

    while (d10_can_platform_receive(&can_id, data, &length)) {
        last_receive_ms = now_ms;
        d10_can_platform_log(true, can_id, data, length);

        if (can_id == D10_CAN_LED_BRIGHTNESS_ID && length == D10_CAN_FRAME_LENGTH) {
            set_led_brightness(data[0]);
        }
    }
}

bool d10_can_link_timed_out(uint32_t now_ms)
{
    return (uint32_t)(now_ms - last_receive_ms) >= D10_CAN_TIMEOUT_MS;
}

uint32_t d10_can_tx_error_count(void)
{
    return tx_errors;
}
