#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../button-matrix/teensy-firmware/d10_button_matrix.h"
#include "../button-matrix/teensy-firmware/d10_can_driver.h"

static uint32_t sent_id;
static uint8_t sent_data[8];
static uint8_t sent_length;
static uint32_t configured_baud;
static uint32_t incoming_id;
static uint8_t incoming_data[8];
static uint8_t incoming_length;
static bool incoming_pending;
static bool transmit_ok = true;
static uint8_t led_value;

bool d10_can_platform_begin(uint32_t baud_rate)
{
    configured_baud = baud_rate;
    return true;
}

bool d10_can_platform_send(uint32_t can_id, const uint8_t *data, uint8_t length)
{
    sent_id = can_id;
    sent_length = length;
    for (uint8_t i = 0; i < length; ++i) {
        sent_data[i] = data[i];
    }
    return transmit_ok;
}

bool d10_can_platform_receive(uint32_t *can_id, uint8_t *data, uint8_t *length)
{
    if (!incoming_pending) {
        return false;
    }
    incoming_pending = false;
    *can_id = incoming_id;
    *length = incoming_length;
    for (uint8_t i = 0; i < incoming_length; ++i) {
        data[i] = incoming_data[i];
    }
    return true;
}

void d10_can_platform_log(bool incoming, uint32_t can_id, const uint8_t *data, uint8_t length)
{
    (void)incoming;
    (void)can_id;
    (void)data;
    (void)length;
}

void d10_led_platform_set(uint8_t brightness)
{
    led_value = brightness;
}

int main(void)
{
    d10_can_driver_init(100u);
    assert(configured_baud == D10_CAN_BAUD_RATE);
    assert(!d10_can_link_timed_out(5099u));
    assert(d10_can_link_timed_out(5100u));

    on_button_pressed(D10_SW20);
    assert(sent_id == D10_CAN_BUTTON_EVENT_ID);
    assert(sent_length == D10_CAN_FRAME_LENGTH);
    assert(sent_data[0] == D10_SW20);
    assert(sent_data[1] == 1u);
    for (uint8_t i = 2; i < sent_length; ++i) {
        assert(sent_data[i] == 0u);
    }

    on_button_released(D10_SW20);
    assert(sent_data[0] == D10_SW20);
    assert(sent_data[1] == 0u);

    transmit_ok = false;
    on_button_pressed(-1);
    on_button_pressed(D10_SW1);
    assert(d10_can_tx_error_count() == 1u);
    transmit_ok = true;

    incoming_id = D10_CAN_LED_BRIGHTNESS_ID;
    incoming_length = D10_CAN_FRAME_LENGTH;
    incoming_data[0] = 128u;
    incoming_pending = true;
    d10_can_service(200u);
    assert(led_value == 128u);
    assert(!d10_can_link_timed_out(5199u));
    assert(d10_can_link_timed_out(5200u));

    puts("CAN driver tests passed");
    return 0;
}
