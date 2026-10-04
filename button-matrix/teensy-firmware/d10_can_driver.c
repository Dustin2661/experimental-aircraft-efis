#include "d10_can_driver.h"
#include "d10_button_matrix.h"

static bool initialized;
static uint32_t tx_errors;

void d10_can_driver_init(void)
{
    tx_errors = 0;
    initialized = d10_can_platform_begin(D10_CAN_BAUD_RATE);
}

bool send_can_message(uint16_t id, const uint8_t *data, uint8_t length)
{
    if (id > 0x7FFu || !data || length > D10_CAN_FRAME_LENGTH) {
        return false;
    }
    if (!initialized || !d10_can_platform_send(id, data, length)) {
        ++tx_errors;
        return false;
    }
    return true;
}

void d10_can_service(void)
{
    uint32_t id;
    uint8_t data[D10_CAN_FRAME_LENGTH];
    uint8_t length;
    /* Bound work per loop so a busy bus cannot starve matrix scanning. */
    for (uint8_t frames = 0; frames < 16u; ++frames) {
        if (!d10_can_platform_receive(&id, data, &length)) {
            break;
        }
        if (id == D10_CAN_LED_BRIGHTNESS_ID && length == D10_CAN_FRAME_LENGTH) {
            set_led_brightness(data[0]);
        }
    }
}

uint32_t d10_can_tx_error_count(void)
{
    return tx_errors;
}
