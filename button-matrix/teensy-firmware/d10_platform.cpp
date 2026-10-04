#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <Watchdog_t4.h>

#include "d10_button_matrix.h"
#include "d10_can_driver.h"
#include "d10_test.h"

static const uint8_t row_pins[D10_ROW_COUNT] = {
    D10_ROW0_PIN, D10_ROW1_PIN, D10_ROW2_PIN, D10_ROW3_PIN, D10_ROW4_PIN
};
static const uint8_t column_pins[D10_COLUMN_COUNT] = {
    D10_COL0_PIN, D10_COL1_PIN, D10_COL2_PIN, D10_COL3_PIN
};

static FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can_bus;
static WDT_T4<WDT1> watchdog;
static IntervalTimer scan_timer;
static volatile bool scan_due;
static CAN_message_t received_frame;

static void scan_timer_isr()
{
    scan_due = true;
}

static void set_matrix_row(uint8_t pin, bool active)
{
    digitalWrite(pin, active ? LOW : HIGH);
}

static bool read_matrix_column(uint8_t pin)
{
    return digitalRead(pin) == HIGH;
}

static void matrix_delay_us(uint32_t microseconds)
{
    delayMicroseconds(microseconds);
}

static const d10_matrix_io_t matrix_io = {
    set_matrix_row,
    read_matrix_column,
    matrix_delay_us
};

bool d10_can_platform_begin(uint32_t baud_rate)
{
    can_bus.begin();
    can_bus.setBaudRate(baud_rate);
    return true;
}

bool d10_can_platform_send(uint32_t can_id, const uint8_t *data, uint8_t length)
{
    CAN_message_t message = {};
    if (length > D10_CAN_FRAME_LENGTH) {
        return false;
    }

    message.id = can_id;
    message.len = length;
    message.flags.extended = 0;
    for (uint8_t i = 0; i < length; ++i) {
        message.buf[i] = data[i];
    }
    return can_bus.write(message);
}

bool d10_can_platform_receive(uint32_t *can_id, uint8_t *data, uint8_t *length)
{
    if (!can_bus.read(received_frame)) {
        return false;
    }

    *can_id = received_frame.flags.extended
                  ? received_frame.id | 0x80000000u
                  : received_frame.id;
    *length = received_frame.len > D10_CAN_FRAME_LENGTH
                  ? D10_CAN_FRAME_LENGTH
                  : received_frame.len;
    for (uint8_t i = 0; i < *length; ++i) {
        data[i] = received_frame.buf[i];
    }
    return true;
}

void d10_can_platform_log(bool incoming, uint32_t can_id, const uint8_t *data, uint8_t length)
{
    Serial.printf("CAN %s %03lX [%u]", incoming ? "RX" : "TX",
                  static_cast<unsigned long>(can_id), length);
    for (uint8_t i = 0; i < length; ++i) {
        Serial.printf(" %02X", data[i]);
    }
    Serial.println();
}

void d10_led_platform_set(uint8_t brightness)
{
    analogWrite(D10_LED_PWM_PIN, brightness);
}

void setup()
{
    Serial.begin(115200);

    for (uint8_t row = 0; row < D10_ROW_COUNT; ++row) {
        pinMode(row_pins[row], OUTPUT);
        digitalWrite(row_pins[row], HIGH);
    }
    for (uint8_t column = 0; column < D10_COLUMN_COUNT; ++column) {
        pinMode(column_pins[column], INPUT_PULLUP);
    }

    pinMode(D10_LED_PWM_PIN, OUTPUT);
    analogWriteFrequency(D10_LED_PWM_PIN, 500);
    analogWrite(D10_LED_PWM_PIN, 0);
    d10_matrix_init();
    d10_can_driver_init(millis());

    WDT_timings_t watchdog_config;
    watchdog_config.timeout = 2.0;
    watchdog.begin(watchdog_config);

    scan_timer.begin(scan_timer_isr, D10_SCAN_INTERVAL_MS * 1000u);
    Serial.println("D10 matrix ready; enter 'r' for an LED ramp test.");
}

void loop()
{
    const uint32_t now_ms = millis();
    static uint32_t last_can_status_ms;

    if (scan_due) {
        noInterrupts();
        const bool should_scan = scan_due;
        scan_due = false;
        interrupts();
        if (should_scan) {
            d10_scan_button_matrix(now_ms, &matrix_io);
        }
    }

    d10_can_service(now_ms);
    if ((uint32_t)(now_ms - last_can_status_ms) >= D10_CAN_TIMEOUT_MS) {
        last_can_status_ms = now_ms;
        Serial.printf("CAN status: RX timeout=%s, TX errors=%lu\n",
                      d10_can_link_timed_out(now_ms) ? "yes" : "no",
                      static_cast<unsigned long>(d10_can_tx_error_count()));
    }

    if (Serial.available() > 0 && Serial.read() == 'r') {
        d10_test_led_ramp_start();
    }
    d10_test_led_ramp_step(now_ms);

    watchdog.feed();
}
