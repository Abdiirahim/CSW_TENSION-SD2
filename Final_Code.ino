#include "Arduino_H7_Video.h"
#include "lvgl.h"
#include "Arduino_GigaDisplayTouch.h"
#include <AccelStepper.h>

// Stepper Motor Pins (Matches CNC Shield)
#define Z_STEP_PIN 4
#define Z_DIR_PIN 7
#define Z2_STEP_PIN 5
#define Z2_DIR_PIN 6
#define STEPPER_ENABLE 8

#define STEPS_PER_MM 100
#define MIN_HEIGHT -40
#define MAX_HEIGHT  40
#define LIMIT_SWITCH_DOWN_PIN 23  // Physical button or limit switch input for -40mm
#define LIMIT_SWITCH_UP_PIN   25  // New: For +40mm

Arduino_H7_Video Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch TouchDetector;

lv_obj_t *main_screen, *menu_screen, *manual_screen;
lv_obj_t *preset1_screen, *preset2_screen;
lv_obj_t *height_label;

static int height = 0;                     // Manual height only
static int total_height = 0;               // Tracks all movements
static bool limit_down_triggered = false;
static bool limit_up_triggered = false;

AccelStepper stepper(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);
AccelStepper stepper2(AccelStepper::DRIVER, Z2_STEP_PIN, Z2_DIR_PIN);

static bool is_inc_pressed = false;
static bool is_dec_pressed = false;
static unsigned long last_step_time = 0;
const unsigned long step_interval = 200;

void moveStepper(int steps, bool direction) {
    int target = direction ? steps : -steps;
    stepper.moveTo(stepper.currentPosition() + target);
    stepper2.moveTo(stepper2.currentPosition() + target);
}

static lv_obj_t *create_button(lv_obj_t *parent, const char *text, lv_event_cb_t event_cb) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 300, 100);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return btn;
}

static void return_menu_event_cb(lv_event_t *e) {
    lv_scr_load(menu_screen);
}

static void preset1_event_cb(lv_event_t *e) {
    moveStepper(0.1 * STEPS_PER_MM, HIGH);
    total_height = 10;
    // height label does not change
}

static void preset2_event_cb(lv_event_t *e) {
    moveStepper(0.2 * STEPS_PER_MM, HIGH);
    total_height = 20;
    // height label does not change
}

// Home button behaves like a reverse preset (smooth motion to 40mm)
static void home_event_cb(lv_event_t *e) {
    moveStepper(0.4 * STEPS_PER_MM, LOW);
    total_height = 40;
    // height label does not change
}

static void increment_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        is_inc_pressed = true;
        last_step_time = 0;
    } else if (code == LV_EVENT_RELEASED) {
        is_inc_pressed = false;
    }
}

static void decrement_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        is_dec_pressed = true;
        last_step_time = 0;
    } else if (code == LV_EVENT_RELEASED) {
        is_dec_pressed = false;
    }
}

static void manual_adjustment_event_cb(lv_event_t *e) {
    lv_scr_load(manual_screen);
}

void setup() {
    Serial.begin(115200);

    pinMode(Z_STEP_PIN, OUTPUT);
    pinMode(Z_DIR_PIN, OUTPUT);
    pinMode(Z2_STEP_PIN, OUTPUT);
    pinMode(Z2_DIR_PIN, OUTPUT);
    pinMode(STEPPER_ENABLE, OUTPUT);
    digitalWrite(STEPPER_ENABLE, LOW);

    pinMode(LIMIT_SWITCH_DOWN_PIN, INPUT_PULLUP);
    pinMode(LIMIT_SWITCH_UP_PIN, INPUT_PULLUP);

    Display.begin();
    TouchDetector.begin();

    stepper.setMaxSpeed(1000);
    stepper.setAcceleration(500);
    stepper2.setMaxSpeed(1000);
    stepper2.setAcceleration(500);

    menu_screen = lv_obj_create(NULL);
    lv_obj_t *btn1 = create_button(menu_screen, "Preset 1 (10mm)", preset1_event_cb);
    lv_obj_align(btn1, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *btn2 = create_button(menu_screen, "Preset 2 (20mm)", preset2_event_cb);
    lv_obj_align_to(btn2, btn1, LV_ALIGN_OUT_BOTTOM_MID, 0, 40);

    lv_obj_t *btn3 = create_button(menu_screen, "Manual Adjustment", manual_adjustment_event_cb);
    lv_obj_align_to(btn3, btn2, LV_ALIGN_OUT_BOTTOM_MID, 0, 40);

    lv_obj_t *btn4 = create_button(menu_screen, " Home (40 mm)", home_event_cb);
    lv_obj_align_to(btn4, btn3, LV_ALIGN_OUT_BOTTOM_MID, 0, 60);

    manual_screen = lv_obj_create(NULL);
    height_label = lv_label_create(manual_screen);
    lv_label_set_text(height_label, "Height: 0 mm");
    lv_obj_align(height_label, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *inc_btn = lv_btn_create(manual_screen);
    lv_obj_set_size(inc_btn, 300, 100);
    lv_obj_add_event_cb(inc_btn, increment_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(inc_btn, increment_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *inc_label = lv_label_create(inc_btn);
    lv_label_set_text(inc_label, "+ Increment");
    lv_obj_center(inc_label);
    lv_obj_align(inc_btn, LV_ALIGN_CENTER, -200, 10);

    lv_obj_t *dec_btn = lv_btn_create(manual_screen);
    lv_obj_set_size(dec_btn, 300, 100);
    lv_obj_add_event_cb(dec_btn, decrement_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(dec_btn, decrement_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t *dec_label = lv_label_create(dec_btn);
    lv_label_set_text(dec_label, "- Decrement");
    lv_obj_center(dec_label);
    lv_obj_align(dec_btn, LV_ALIGN_CENTER, 200, 10);

    create_button(manual_screen, "Return to Menu", return_menu_event_cb);

    lv_scr_load(menu_screen);
}

void loop() {
    lv_timer_handler();
    stepper.run();
    stepper2.run();

    unsigned long current_time = millis();

    // LIMIT SWITCH 40mm
    if (digitalRead(LIMIT_SWITCH_DOWN_PIN) == LOW && !limit_down_triggered) {
        moveStepper(0.4 * STEPS_PER_MM, LOW);
        total_height = 40;
        limit_down_triggered = true;
    }
    if (digitalRead(LIMIT_SWITCH_DOWN_PIN) == HIGH) {
        limit_down_triggered = false;
    }

    // LIMIT SWITCH +40mm
    if (digitalRead(LIMIT_SWITCH_UP_PIN) == LOW && !limit_up_triggered) {
        moveStepper(0.4 * STEPS_PER_MM, HIGH);
        total_height = 40;
        limit_up_triggered = true;
    }
    if (digitalRead(LIMIT_SWITCH_UP_PIN) == HIGH) {
        limit_up_triggered = false;
    }

    if ((is_inc_pressed || is_dec_pressed) && current_time - last_step_time >= step_interval) {
        if (is_inc_pressed && height < MAX_HEIGHT) {
            moveStepper(0.05 * STEPS_PER_MM, HIGH);
            height++;
        }
        if (is_dec_pressed && height > MIN_HEIGHT) {
            moveStepper(0.05 * STEPS_PER_MM, LOW);
            height--;
        }
        lv_label_set_text_fmt(height_label, "Height: %d mm", height);
        last_step_time = current_time;
    }

    delay(5);
}


