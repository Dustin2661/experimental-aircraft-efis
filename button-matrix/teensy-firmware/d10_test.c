#include "d10_button_matrix.h"
#include "d10_test.h"

static uint32_t last_step_ms;
static uint8_t brightness;
static int8_t direction = 1;
static bool ramp_active;

void d10_test_led_ramp_start(void)
{
    brightness = 0;
    direction = 1;
    last_step_ms = 0;
    ramp_active = true;
}

void d10_test_led_ramp_step(uint32_t now_ms)
{
    if (!ramp_active || (uint32_t)(now_ms - last_step_ms) < 10u) {
        return;
    }

    last_step_ms = now_ms;
    set_led_brightness(brightness);

    if (brightness == 255u) {
        direction = -1;
    } else if (brightness == 0u) {
        direction = 1;
    }
    brightness = (uint8_t)(brightness + direction);
}
