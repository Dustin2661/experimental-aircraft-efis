#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include "button_matrix.h"

static const uint8_t ROW_PINS[BM_ROWS] = {0, 1, 2, 3};
static const uint8_t COLUMN_PINS[BM_COLUMNS] = {5, 6, 7, 8, 9};
static const uint8_t LED_SIN_PIN = 11;
static const uint8_t LED_SCLK_PIN = 13;
static const uint8_t LED_XLAT_PIN = 10;
static const uint8_t LED_CHANNELS = 24;
static const uint8_t LED_BITS = 12;
static const uint8_t SERIAL_BUFFER_SIZE = 48;

static bm_t matrix;
static uint16_t led_values[LED_CHANNELS] = {0};
static char serial_buffer[SERIAL_BUFFER_SIZE];
static uint8_t serial_length = 0;
static uint32_t last_scan_ms = 0;

static void write_led_driver()
{
    int8_t channel;
    int8_t bit;

    for (channel = LED_CHANNELS - 1; channel >= 0; --channel) {
        for (bit = LED_BITS - 1; bit >= 0; --bit) {
            digitalWrite(LED_SCLK_PIN, LOW);
            digitalWrite(LED_SIN_PIN, (led_values[channel] & (1U << bit)) ? HIGH : LOW);
            digitalWrite(LED_SCLK_PIN, HIGH);
        }
    }
    digitalWrite(LED_SCLK_PIN, LOW);
    digitalWrite(LED_XLAT_PIN, HIGH);
    digitalWrite(LED_XLAT_PIN, LOW);
}

static void set_row_active(void *, uint8_t row, uint8_t active)
{
    for (uint8_t index = 0; index < BM_ROWS; ++index) {
        digitalWrite(ROW_PINS[index],
                     (index == row && active) ? LOW : HIGH);
    }
}

static uint8_t read_column(void *, uint8_t column)
{
    return (uint8_t)digitalRead(COLUMN_PINS[column]);
}

static uint32_t get_millis(void *)
{
    return millis();
}

static void wait_us(void *, uint16_t microseconds)
{
    delayMicroseconds(microseconds);
}

static void report_button(void *, uint8_t button, uint8_t pressed)
{
    Serial.print("BUTTON ");
    Serial.print(button + 1);
    Serial.println(pressed ? " DOWN" : " UP");
}

static void set_led(void *, uint8_t button, uint16_t brightness)
{
    led_values[button] = brightness;
    write_led_driver();
}

static void process_command(char *line)
{
    char *command = strtok(line, " ");
    if (command == NULL) {
        return;
    }
    if (strcmp(command, "TEST") == 0) {
        Serial.println("TEST OK");
        return;
    }
    if (strcmp(command, "LED") == 0) {
        char *id_text = strtok(NULL, " ");
        char *brightness_text = strtok(NULL, " ");
        if (id_text != NULL && brightness_text != NULL) {
            const long id = strtol(id_text, NULL, 10);
            const long brightness = strtol(brightness_text, NULL, 10);
            if (id >= 1 && id <= BM_BUTTONS && brightness >= 0 && brightness <= 4095) {
                bm_set_led(&matrix, (uint8_t)(id - 1), (uint16_t)brightness);
                Serial.print("LED ");
                Serial.print(id);
                Serial.print(" ");
                Serial.println(brightness);
                return;
            }
        }
    }
    Serial.println("ERROR");
}

static void service_serial()
{
    while (Serial.available() > 0) {
        const char character = (char)Serial.read();
        if (character == '\n' || character == '\r') {
            if (serial_length > 0) {
                serial_buffer[serial_length] = '\0';
                process_command(serial_buffer);
                serial_length = 0;
            }
        } else if (serial_length < SERIAL_BUFFER_SIZE - 1) {
            serial_buffer[serial_length++] = character;
        } else {
            serial_length = 0;
            Serial.println("ERROR");
        }
    }
}

void setup()
{
    Serial.begin(115200);
    for (uint8_t row = 0; row < BM_ROWS; ++row) {
        pinMode(ROW_PINS[row], OUTPUT);
        digitalWrite(ROW_PINS[row], HIGH);
    }
    for (uint8_t column = 0; column < BM_COLUMNS; ++column) {
        pinMode(COLUMN_PINS[column], INPUT_PULLUP);
    }
    pinMode(LED_SIN_PIN, OUTPUT);
    pinMode(LED_SCLK_PIN, OUTPUT);
    pinMode(LED_XLAT_PIN, OUTPUT);
    digitalWrite(LED_SIN_PIN, LOW);
    digitalWrite(LED_SCLK_PIN, LOW);
    digitalWrite(LED_XLAT_PIN, LOW);
    write_led_driver();

    const bm_io_t io = {
        NULL, set_row_active, read_column, get_millis, wait_us,
        report_button, set_led
    };
    bm_init(&matrix, &io);
    Serial.println("READY");
}

void loop()
{
    service_serial();
    const uint32_t now = millis();
    if ((uint32_t)(now - last_scan_ms) >= 10) {
        last_scan_ms = now;
        bm_scan(&matrix);
    }
}
