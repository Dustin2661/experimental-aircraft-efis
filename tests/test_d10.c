#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../button-matrix/teensy-firmware/d10_button_matrix.h"
#include "../button-matrix/teensy-firmware/d10_can_driver.h"

static bool switches[TOTAL_BUTTONS];
static int active_row = -1;
static uint16_t sent_id[128];
static uint8_t sent_event[128];
static unsigned sent_count;
static bool begin_ok = true;
static bool transmit_ok = true;
static bool receive_pending;
static uint32_t receive_id;
static uint8_t receive_length;
static uint8_t receive_brightness;
static uint8_t pwm;

bool d10_can_platform_begin(uint32_t baud)
{
    assert(baud == D10_CAN_BAUD_RATE);
    return begin_ok;
}

bool d10_can_platform_send(uint16_t id, const uint8_t *data, uint8_t length)
{
    assert(length == 8);
    assert(data[0] < TOTAL_BUTTONS);
    assert(id == CAN_ID_LIDAR + data[0]);
    for (unsigned i = 2; i < length; ++i) {
        assert(data[i] == 0);
    }
    assert(sent_count < 128);
    sent_id[sent_count] = id;
    sent_event[sent_count++] = data[1];
    return transmit_ok;
}

bool d10_can_platform_receive(uint32_t *id, uint8_t *data, uint8_t *length)
{
    if (!receive_pending) {
        return false;
    }
    receive_pending = false;
    *id = receive_id;
    *length = receive_length;
    memset(data, 0, 8);
    data[0] = receive_brightness;
    return true;
}

void d10_led_platform_set(uint8_t brightness)
{
    pwm = brightness;
}

static void set_row(uint8_t pin, bool active)
{
    assert(pin < BUTTON_ROWS);
    if (active) {
        assert(active_row == -1);
        active_row = pin;
    } else {
        assert(active_row == pin);
        active_row = -1;
    }
}

static bool read_column(uint8_t pin)
{
    assert(active_row >= 0);
    assert(pin >= COL0_PIN && pin <= COL3_PIN);
    return !switches[active_row * BUTTON_COLS + pin - COL0_PIN];
}

static void delay_us(uint32_t us)
{
    assert(us == D10_ROW_SETTLE_US);
}

static const d10_matrix_io_t io = {set_row, read_column, delay_us};

static void reset(void)
{
    memset(switches, 0, sizeof(switches));
    sent_count = 0;
    button_matrix_init();
    d10_can_driver_init();
}

static void scan(uint32_t now)
{
    scan_button_matrix(now, &io);
    assert(active_row == -1);
}

static void test_all_buttons(void)
{
    for (unsigned id = 0; id < TOTAL_BUTTONS; ++id) {
        reset();
        switches[id] = true;
        scan(100);
        scan(119);
        assert(sent_count == 0);
        scan(120);
        assert(sent_count == 1 && sent_id[0] == CAN_ID_LIDAR + id);
        assert(sent_event[0] == 1);
        assert(d10_button_state((button_id_t)id) == BTN_STATE_PRESSED);
        scan(130);
        assert(sent_count == 1);
        switches[id] = false;
        scan(140);
        scan(160);
        assert(sent_count == 2 && sent_event[1] == 0);
        assert(d10_button_state((button_id_t)id) == BTN_STATE_RELEASED);
        scan(165);
        assert(d10_button_state((button_id_t)id) == BTN_STATE_IDLE);
    }
}

static void test_hold_and_rollover(void)
{
    reset();
    switches[BTN_MENU_DIM] = true;
    scan(UINT32_MAX - 30u);
    scan(UINT32_MAX - 10u);
    scan(489u); /* Exactly 500 ms since confirmed press. */
    assert(sent_count == 1);
    scan(490u);
    assert(sent_count == 2 && sent_event[1] == 2);
    assert(d10_button_state(BTN_MENU_DIM) == BTN_STATE_HELD);
    scan(1000u);
    assert(sent_count == 2);
    switches[BTN_MENU_DIM] = false;
    scan(1005u);
    scan(1025u);
    assert(sent_count == 3 && sent_event[2] == 0);

    reset();
    switches[BTN_MENU_DIM] = true;
    scan(0);
    scan(20);
    switches[BTN_MENU_DIM] = false;
    scan(520);
    scan(540); /* Release debounce must not invent a hold. */
    assert(sent_count == 2 && sent_event[1] == 0);
}

static void test_bounce_and_simultaneous(void)
{
    reset();
    switches[BTN_LIDAR] = true;
    scan(UINT32_MAX - 10u);
    switches[BTN_LIDAR] = false;
    scan(0);
    scan(20);
    assert(sent_count == 0);
    switches[BTN_LIDAR] = switches[BTN_PFD] = true;
    scan(UINT32_MAX - 10u);
    scan(10);
    assert(sent_count == 2);
    scan(1000);
    assert(sent_count == 2); /* Non-MENU buttons never repeat on hold. */
}

static void test_validation_and_brightness(void)
{
    reset();
    scan_button_matrix(0, NULL);
    const d10_matrix_io_t invalid = {NULL, read_column, delay_us};
    scan_button_matrix(0, &invalid);
    on_button_pressed((button_id_t)-1);
    on_button_pressed((button_id_t)256);
    on_button_released((button_id_t)TOTAL_BUTTONS);
    on_button_held(BTN_LIDAR);
    assert(sent_count == 0);
    assert(d10_button_state((button_id_t)-1) == BTN_STATE_IDLE);
    transmit_ok = false;
    on_button_pressed(BTN_AUTOPILOT);
    assert(d10_can_tx_error_count() == 1);
    transmit_ok = true;
    begin_ok = false;
    d10_can_driver_init();
    on_button_pressed(BTN_CM5_BOOT);
    assert(d10_can_tx_error_count() == 1);
    begin_ok = true;
    d10_can_driver_init();
    const uint8_t data[8] = {0};
    assert(!send_can_message(0x800u, data, 8));
    assert(!send_can_message(0x601u, NULL, 8));
    assert(!send_can_message(0x601u, data, 9));

    set_led_brightness(0);
    assert(pwm == 0 && d10_led_brightness() == 0);
    set_led_brightness(255);
    assert(pwm == 255 && d10_led_brightness() == 255);
    receive_id = D10_CAN_LED_BRIGHTNESS_ID;
    receive_length = 7;
    receive_brightness = 128;
    receive_pending = true;
    d10_can_service();
    assert(pwm == 255);
    receive_length = 8;
    receive_id |= 0x80000000u;
    receive_pending = true;
    d10_can_service();
    assert(pwm == 255);
    receive_id = D10_CAN_LED_BRIGHTNESS_ID;
    receive_pending = true;
    d10_can_service();
    assert(pwm == 128 && d10_led_brightness() == 128);
}

int main(void)
{
    test_all_buttons();
    test_hold_and_rollover();
    test_bounce_and_simultaneous();
    test_validation_and_brightness();
    puts("D10 mapping, debounce, hold, CAN and brightness tests passed");
    return 0;
}
