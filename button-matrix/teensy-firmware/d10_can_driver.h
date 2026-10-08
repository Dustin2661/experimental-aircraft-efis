#ifndef D10_CAN_DRIVER_H
#define D10_CAN_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#define D10_CAN_BUTTON_EVENT_ID 0x600u
#define D10_CAN_LED_BRIGHTNESS_ID 0x620u
#define D10_CAN_BAUD_RATE 500000u
#define D10_CAN_FRAME_LENGTH 8u
#define D10_CAN_TIMEOUT_MS 5000u

void d10_can_driver_init(uint32_t now_ms);
void d10_can_service(uint32_t now_ms);
bool d10_can_link_timed_out(uint32_t now_ms);
uint32_t d10_can_tx_error_count(void);

bool d10_can_platform_begin(uint32_t baud_rate);
bool d10_can_platform_send(uint32_t can_id, const uint8_t *data, uint8_t length);
bool d10_can_platform_receive(uint32_t *can_id, uint8_t *data, uint8_t *length);
void d10_can_platform_log(bool incoming, uint32_t can_id, const uint8_t *data, uint8_t length);
void d10_led_platform_set(uint8_t brightness);

#endif
