#ifndef D10_CAN_DRIVER_H
#define D10_CAN_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define D10_CAN_BAUD_RATE 500000u
#define D10_CAN_FRAME_LENGTH 8u
#define D10_CAN_LED_BRIGHTNESS_ID 0x620u

void d10_can_driver_init(void);
bool send_can_message(uint16_t id, const uint8_t *data, uint8_t length);
void d10_can_service(void);
uint32_t d10_can_tx_error_count(void);
bool d10_can_platform_begin(uint32_t baud);
bool d10_can_platform_send(uint16_t id, const uint8_t *data, uint8_t length);
bool d10_can_platform_receive(uint32_t *id, uint8_t *data, uint8_t *length);
void d10_led_platform_set(uint8_t brightness);

#ifdef __cplusplus
}
#endif
#endif
