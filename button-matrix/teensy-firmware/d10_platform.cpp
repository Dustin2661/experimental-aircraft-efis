#include <Arduino.h>
#include <FlexCAN_T4.h>

#include "d10_button_matrix.h"
#include "d10_can_driver.h"

static const uint8_t rows[BUTTON_ROWS] = {
    ROW0_PIN, ROW1_PIN, ROW2_PIN, ROW3_PIN, ROW4_PIN
};
static const uint8_t columns[BUTTON_COLS] = {
    COL0_PIN, COL1_PIN, COL2_PIN, COL3_PIN
};
static FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can_bus;

static void set_row(uint8_t pin, bool active)
{
    if (active) {
        digitalWrite(pin, LOW);
        pinMode(pin, OUTPUT);
    } else {
        pinMode(pin, INPUT);
    }
}

static bool read_column(uint8_t pin)
{
    return digitalRead(pin) == HIGH;
}

static void delay_us(uint32_t us)
{
    delayMicroseconds(us);
}

static const d10_matrix_io_t matrix_io = {set_row, read_column, delay_us};

bool d10_can_platform_begin(uint32_t baud)
{
    can_bus.begin();
    can_bus.setBaudRate(baud);
    return true;
}

bool d10_can_platform_send(uint16_t id, const uint8_t *data, uint8_t length)
{
    CAN_message_t message = {};
    message.id = id;
    message.len = length;
    for (uint8_t i = 0; i < length; ++i) {
        message.buf[i] = data[i];
    }
    return can_bus.write(message) != 0;
}

bool d10_can_platform_receive(uint32_t *id, uint8_t *data, uint8_t *length)
{
    CAN_message_t message;
    if (!can_bus.read(message)) {
        return false;
    }
    *id = message.flags.extended || message.flags.remote
              ? 0xFFFFFFFFu : message.id;
    *length = message.len > 8u ? 8u : message.len;
    for (uint8_t i = 0; i < *length; ++i) {
        data[i] = message.buf[i];
    }
    return true;
}

void d10_led_platform_set(uint8_t brightness)
{
    analogWrite(LED_PWM_PIN, brightness);
}

void setup()
{
    for (uint8_t pin : rows) {
        pinMode(pin, INPUT);
    }
    for (uint8_t pin : columns) {
        pinMode(pin, INPUT_PULLUP);
    }
    digitalWrite(LED_PWM_PIN, LOW);
    pinMode(LED_PWM_PIN, OUTPUT);
    analogWriteResolution(8);
    analogWriteFrequency(LED_PWM_PIN, 500);
    set_led_brightness(128u);
    button_matrix_init();
    d10_can_driver_init();
}

void loop()
{
    static uint32_t last_scan;
    const uint32_t now = millis();
    if ((uint32_t)(now - last_scan) >= D10_SCAN_INTERVAL_MS) {
        last_scan = now;
        scan_button_matrix(now, &matrix_io);
    }
    d10_can_service();
}
