#ifndef D10_BUTTON_MATRIX_H
#define D10_BUTTON_MATRIX_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ROW0_PIN 0u /* J1-9 */
#define ROW1_PIN 1u /* J1-10 */
#define ROW2_PIN 2u /* J1-12 */
#define ROW3_PIN 3u /* J1-2 */
#define ROW4_PIN 4u /* J1-4 */
#define COL0_PIN 5u /* J1-1 */
#define COL1_PIN 6u /* J1-3 */
#define COL2_PIN 7u /* J1-5 */
#define COL3_PIN 8u /* J1-7 */
#define LED_PWM_PIN 9u /* External 3.3 V-compatible high-side driver only. */
#define BUTTON_ROWS 5u
#define BUTTON_COLS 4u
#define TOTAL_BUTTONS 20u
#define DEBOUNCE_MS 20u
#define HOLD_THRESHOLD_MS 500u
#define D10_SCAN_INTERVAL_MS 5u
#define D10_ROW_SETTLE_US 50u

typedef enum {
    BTN_LIDAR = 0, BTN_AUTOPILOT, BTN_EMERG_COM, BTN_CM5_BOOT,
    BTN_MAP, BTN_COM_EDIT, BTN_NAV_EDIT, BTN_ENGINE_MON,
    BTN_TRAFFIC, BTN_PFD, BTN_DIRECT_TO, BTN_FLIGHT_PLAN,
    BTN_AIRPORT, BTN_COM_SWAP, BTN_NAV_SWAP, BTN_XPDR,
    BTN_BARO, BTN_ALERT_ACK, BTN_BACK, BTN_MENU_DIM
} button_id_t;

#define CAN_ID_BUTTON_EVENT 0x600u
#define CAN_ID_LIDAR 0x601u
#define CAN_ID_AUTOPILOT 0x602u
#define CAN_ID_EMERG_COM 0x603u
#define CAN_ID_CM5_BOOT 0x604u
#define CAN_ID_MAP 0x605u
#define CAN_ID_COM_EDIT 0x606u
#define CAN_ID_NAV_EDIT 0x607u
#define CAN_ID_ENGINE_MON 0x608u
#define CAN_ID_TRAFFIC 0x609u
#define CAN_ID_PFD 0x60Au
#define CAN_ID_DIRECT_TO 0x60Bu
#define CAN_ID_FLIGHT_PLAN 0x60Cu
#define CAN_ID_AIRPORT 0x60Du
#define CAN_ID_COM_SWAP 0x60Eu
#define CAN_ID_NAV_SWAP 0x60Fu
#define CAN_ID_XPDR 0x610u
#define CAN_ID_BARO 0x611u
#define CAN_ID_ALERT_ACK 0x612u
#define CAN_ID_BACK 0x613u
#define CAN_ID_MENU_DIM 0x614u

typedef enum {
    BTN_STATE_IDLE = 0, BTN_STATE_PRESSED, BTN_STATE_HELD, BTN_STATE_RELEASED
} button_state_t;

typedef struct {
    void (*set_row)(uint8_t pin, bool active);
    bool (*read_column)(uint8_t pin);
    void (*delay_us)(uint32_t microseconds);
} d10_matrix_io_t;

void button_matrix_init(void);
void scan_button_matrix(uint32_t now_ms, const d10_matrix_io_t *io);
button_state_t d10_button_state(button_id_t id);
void set_led_brightness(uint8_t brightness);
uint8_t d10_led_brightness(void);
void on_button_pressed(button_id_t id);
void on_button_released(button_id_t id);
void on_button_held(button_id_t id);

#ifdef __cplusplus
}
#endif
#endif
