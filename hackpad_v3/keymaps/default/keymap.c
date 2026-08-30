#include QMK_KEYBOARD_H

enum custom_keycodes {
    CYCLE_LAYERS = SAFE_RANGE,
    OPEN_SPOTIFY,
};

enum layer_names {
    _BASE = 0,
    _LAYER1,
    _LAYER2,
    _LAYER3,
};

#ifdef RGBLIGHT_ENABLE
static uint32_t wave_timer = 0;
static bool wave_active = false;
static uint8_t wave_step = 0;

static void set_first_led_layer(uint8_t layer) {
    // HSV picks: bright, evenly-spaced hues that read well on a single LED.
    switch (layer) {
        case _BASE:
            rgblight_sethsv_at(140, 255, 200, 0); // LED 0 Cyan/teal
            break;
        case _LAYER1:
            rgblight_sethsv_at(240, 255, 200, 0); // LED 0 Violet/magenta
            break;
        case _LAYER2:
            rgblight_sethsv_at(85,  255, 200, 0); // LED 0 Spring green
            break;
        case _LAYER3:
            rgblight_sethsv_at(30,  255, 220, 0); // LED 0 Amber/gold
            break;
        default:
            rgblight_setrgb_at(0, 0, 0, 0);
            break;
    }
}

void trigger_wave_animation(void) {
    wave_active = true;
    wave_step = 0;
    wave_timer = timer_read32();
}

#define PULSE_HUE         16   // orange in QMK's 0-255 hue space
#define PULSE_SAT         255
#define PULSE_MAX_VALUE   255
#define PULSE_FIRST_LED   1    // 2nd LED (0-indexed)
#define PULSE_LAST_LED    5    // 6th LED (0-indexed)
#define PULSE_TRAIL_WIDTH 2    // how many LEDs behind the front stay lit
#define PULSE_STEPS       ((PULSE_LAST_LED - PULSE_FIRST_LED) + PULSE_TRAIL_WIDTH + 2)

void step_wave_animation(void) {
    if (!wave_active) return;

    if (timer_elapsed32(wave_timer) > 35) {
        wave_timer = timer_read32();

        for (uint8_t i = PULSE_FIRST_LED; i <= PULSE_LAST_LED; i++) {
            // distance behind the current wave front
            int16_t dist = (int16_t)wave_step - (int16_t)(i - PULSE_FIRST_LED);

            if (dist < 0 || dist > PULSE_TRAIL_WIDTH) {
                rgblight_setrgb_at(0, 0, 0, i);
            } else {
                // brightest at the front (dist == 0), fading out over the trail
                uint8_t value = (uint16_t)PULSE_MAX_VALUE * (PULSE_TRAIL_WIDTH - dist + 1) / (PULSE_TRAIL_WIDTH + 1);
                rgblight_sethsv_at(PULSE_HUE, PULSE_SAT, value, i);
            }
        }

        wave_step++;

        if (wave_step > PULSE_STEPS) {
            wave_active = false;
            for (uint8_t i = PULSE_FIRST_LED; i <= PULSE_LAST_LED; i++) {
                rgblight_setrgb_at(0, 0, 0, i);
            }
        }
    }
}
#endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
#ifdef RGBLIGHT_ENABLE
        trigger_wave_animation();
#endif
    }

    switch (keycode) {
        case CYCLE_LAYERS:
            if (record->event.pressed) {
                uint8_t current = get_highest_layer(layer_state);
                switch (current) {
                    case _BASE:
                        layer_move(_LAYER2);
                        break;
                    case _LAYER2:
                        layer_move(_LAYER3);
                        break;
                    case _LAYER3:
                    default:
                        layer_move(_BASE);
                        break;
                }
            }
            return false;

        case OPEN_SPOTIFY:
            if (record->event.pressed) {
                SEND_STRING(SS_LGUI("r") SS_DELAY(150));
                tap_code(KC_S);
                tap_code(KC_P);
                tap_code(KC_O);
                tap_code(KC_T);
                tap_code(KC_I);
                tap_code(KC_F);
                tap_code(KC_Z);
                tap_code(KC_ENTER);
            }
            return false;
    }
    return true;
}

void matrix_scan_user(void) {
#ifdef RGBLIGHT_ENABLE
    step_wave_animation();
#endif
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // Layer 0: Media playback (volume up/down handled by the encoder knob)
    [_BASE] = LAYOUT(
        MO(_LAYER1),    KC_MUTE,        CYCLE_LAYERS,
        KC_MPRV,        KC_MPLY,        KC_MNXT,
        KC_MSTP,        KC_MSEL,        OPEN_SPOTIFY
    ),

    // Layer 1: Function keys
    [_LAYER1] = LAYOUT(
        KC_F13,       KC_F14,           KC_F15,
        KC_F16,       KC_F17,           KC_F18,
        KC_F19,       KC_F20,           KC_F21
    ),

    // Layer 2: System / task management
    [_LAYER2] = LAYOUT(
        LCTL(LSFT(KC_ESC)), LGUI(KC_R),      CYCLE_LAYERS,   // Task Manager, Run
        LGUI(KC_E),          LALT(KC_TAB),   LGUI(KC_D),     // Explorer, Alt-Tab, Show Desktop
        LGUI(KC_L),          LALT(KC_F4),    LCTL(LALT(KC_DEL)) // Lock, Close window, Ctrl+Alt+Del
    ),

    // Layer 3: Window/tab management & clipboard
    [_LAYER3] = LAYOUT(
        LGUI(KC_LEFT),  LGUI(KC_UP),    CYCLE_LAYERS,   // Snap left, Snap/maximize
        LGUI(KC_RGHT),  LGUI(KC_DOWN),  LCTL(KC_W),     // Snap right, Minimize, Close tab
        LCTL(KC_Z),     LCTL(KC_C),     LCTL(KC_V)      // Undo, Copy, Paste
    ),
};

#ifdef ENCODER_ENABLE
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = {ENCODER_CCW_CW(KC_VOLU, KC_VOLD)  },
    [1] = {ENCODER_CCW_CW(UG_SATD, UG_SATU)  },
    [2] = {ENCODER_CCW_CW(UG_SPDD, UG_SPDU)  },
    [3] = {ENCODER_CCW_CW(KC_RIGHT, KC_LEFT) },
};
#endif
#endif

#ifdef RGBLIGHT_ENABLE

void keyboard_post_init_user(void) {
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    set_first_led_layer(get_highest_layer(layer_state));
}

#endif

#ifdef OLED_ENABLE

#define OLED_FLASH_DURATION 1000
#define ANIM_FRAME_DURATION 60

static bool     oled_flashing    = false;
static uint32_t oled_flash_timer = 0;
static uint32_t anim_timer       = 0;

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_0;
}

static void render_layer_name(void) {
    oled_set_cursor(0, 0);
    oled_write_P(PSTR("Layer: "), false);
    oled_set_cursor(0, 1);
    switch (get_highest_layer(layer_state)) {
        case _BASE:   oled_write_P(PSTR("Base"), false); break;
        case _LAYER1: oled_write_P(PSTR("1"), false);    break;
        case _LAYER2: oled_write_P(PSTR("2"), false);    break;
        case _LAYER3: oled_write_P(PSTR("3"), false);    break;
        default:      oled_write_P(PSTR("?"), false);    break;
    }
}

static void render_idle_animation(void) {
    static int8_t x = 0, y = 0;
    static int8_t dx = 1, dy = 1;

    if (timer_elapsed32(anim_timer) < ANIM_FRAME_DURATION) {
        return;
    }
    anim_timer = timer_read32();

    oled_clear();

    if (x + dx < 0 || x + dx > (OLED_DISPLAY_WIDTH  - 4)) dx = -dx;
    if (y + dy < 0 || y + dy > (OLED_DISPLAY_HEIGHT - 4)) dy = -dy;
    x += dx;
    y += dy;

    for (int8_t px = 0; px < 4; px++) {
        for (int8_t py = 0; py < 4; py++) {
            oled_write_pixel(x + px, y + py, true);
        }
    }

    render_layer_name();
}

bool oled_task_user(void) {
    if (oled_flashing) {
        if (timer_elapsed32(oled_flash_timer) < OLED_FLASH_DURATION) {
            oled_invert(true);
        } else {
            oled_invert(false);
            oled_flashing = false;
        }
        return false;
    }

    render_idle_animation();
    return false;
}

static void start_oled_flash(void) {
    oled_flashing    = true;
    oled_flash_timer = timer_read32();
}

#endif

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t layer = get_highest_layer(state);

#ifdef RGBLIGHT_ENABLE
    set_first_led_layer(layer);
#endif

#ifdef OLED_ENABLE
    start_oled_flash();
#endif

    return state;
}
