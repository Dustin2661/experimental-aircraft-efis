#include "d10_button_assignments.h"

// Bench demonstration only: no aircraft controls or CM5 boot pins are driven.
constexpr uint8_t NUM_ROWS = 5;
constexpr uint8_t NUM_COLS = 4;
constexpr uint8_t row_pins[NUM_ROWS] = {0, 1, 2, 3, 4};
constexpr uint8_t col_pins[NUM_COLS] = {5, 6, 7, 8};
constexpr uint8_t led_pwm_pin = 9;
constexpr uint32_t SCAN_MS = 10;
constexpr uint32_t DEBOUNCE_MS = 20;
constexpr uint32_t HOLD_MS = 500;
static_assert(NUM_ROWS * NUM_COLS == TOTAL_BUTTONS, "Matrix size mismatch");

struct ButtonState {
  bool raw = false;
  bool confirmed = false;
  bool hold_sent = false;
  uint32_t raw_changed_at = 0;
  uint32_t pressed_at = 0;
};

ButtonState button_states[TOTAL_BUTTONS];
uint32_t last_scan_time = 0;
uint8_t current_brightness = 0;
bool scan_debug = false;

void print_event(uint8_t id, const char* event) {
  Serial.print('[');
  Serial.print(event);
  Serial.print("] Button ");
  Serial.print(id);
  Serial.print(" (");
  Serial.print(button_labels[id]);
  Serial.println(')');
}

void on_button_pressed(uint8_t id) {
  print_event(id, "PRESS");
  // Add application requests here; this template only logs events.
  switch (id) {
    case BTN_LIDAR:
      Serial.println("  -> LiDAR toggle request (not connected)");
      break;
    case BTN_AUTOPILOT:
    case BTN_EMERG_COM:
    case BTN_CM5_BOOT:
      Serial.println("  -> Bench event only; no safety-critical output");
      break;
    default:
      break;
  }
}

void on_button_released(uint8_t id) {
  print_event(id, "RELEASE");
}

void on_button_held(uint8_t id) {
  print_event(id, "HELD");
  if (id == BTN_MENU_DIM) {
    Serial.println("  -> Use 0/1/2/3 in Serial Monitor to test brightness");
  }
}

void process_button_state(uint8_t id, bool pressed, uint32_t now) {
  ButtonState& btn = button_states[id];
  if (pressed != btn.raw) {
    btn.raw = pressed;
    btn.raw_changed_at = now;
  }
  if (btn.raw != btn.confirmed &&
      uint32_t(now - btn.raw_changed_at) >= DEBOUNCE_MS) {
    btn.confirmed = btn.raw;
    if (btn.confirmed) {
      btn.pressed_at = now;
      btn.hold_sent = false;
      on_button_pressed(id);
    } else {
      on_button_released(id);
    }
  }
  if (btn.confirmed && btn.raw && !btn.hold_sent &&
      uint32_t(now - btn.pressed_at) >= HOLD_MS) {
    btn.hold_sent = true;
    on_button_held(id);
  }
}

void scan_button_matrix() {
  bool readings[TOTAL_BUTTONS];
  for (uint8_t row = 0; row < NUM_ROWS; ++row) {
    // Only the selected row drives LOW; all others remain high impedance.
    digitalWrite(row_pins[row], LOW);
    pinMode(row_pins[row], OUTPUT);
    delayMicroseconds(50);
    for (uint8_t col = 0; col < NUM_COLS; ++col) {
      readings[row * NUM_COLS + col] = digitalRead(col_pins[col]) == LOW;
    }
    pinMode(row_pins[row], INPUT);
  }

  const uint32_t now = millis();
  for (uint8_t id = 0; id < TOTAL_BUTTONS; ++id) {
    process_button_state(id, readings[id], now);
  }

  // Limit diagnostics so USB logging does not dominate matrix scanning.
  static uint32_t last_debug_time = 0;
  if (scan_debug && uint32_t(now - last_debug_time) >= 250) {
    last_debug_time = now;
    for (uint8_t row = 0; row < NUM_ROWS; ++row) {
      Serial.print("[SCAN] R");
      Serial.print(row);
      Serial.print(" C0..3=");
      for (uint8_t col = 0; col < NUM_COLS; ++col) {
        Serial.print(readings[row * NUM_COLS + col] ? '1' : '0');
      }
      Serial.println();
    }
  }
}

void set_led_brightness(uint8_t brightness) {
  current_brightness = brightness;
  analogWrite(led_pwm_pin, brightness);
  Serial.print("[LED] Brightness ");
  Serial.print(brightness);
  Serial.print(" (");
  Serial.print(uint16_t(brightness) * 100 / 255);
  Serial.println("%)");
}

void print_help() {
  Serial.println("D10 bench: 5 rows x 4 columns; no aircraft outputs");
  Serial.println("0=off, 1=25%, 2=50%, 3=100%, s=scan debug, ?=help");
}

void setup() {
  for (uint8_t row = 0; row < NUM_ROWS; ++row) {
    pinMode(row_pins[row], INPUT);
    digitalWrite(row_pins[row], LOW);
  }
  for (uint8_t col = 0; col < NUM_COLS; ++col) {
    pinMode(col_pins[col], INPUT_PULLUP);
  }
  digitalWrite(led_pwm_pin, LOW);
  pinMode(led_pwm_pin, OUTPUT);
  analogWriteResolution(8);
  analogWriteFrequency(led_pwm_pin, 500);
  analogWrite(led_pwm_pin, 0);

  Serial.begin(115200);
  const uint32_t start = millis();
  while (!Serial && uint32_t(millis() - start) < 2000) {
    delay(1);
  }
  print_help();
  set_led_brightness(0);
}

void loop() {
  const uint32_t now = millis();
  if (uint32_t(now - last_scan_time) >= SCAN_MS) {
    last_scan_time = now;
    scan_button_matrix();
  }
  // Process at most one byte per loop; CR/LF and unknown input are ignored.
  if (Serial.available()) {
    switch (Serial.read()) {
      case '0': set_led_brightness(0); break;
      case '1': set_led_brightness(64); break;
      case '2': set_led_brightness(128); break;
      case '3': set_led_brightness(255); break;
      case 's': scan_debug = !scan_debug; break;
      case '?': print_help(); break;
      default: break;
    }
  }
}
