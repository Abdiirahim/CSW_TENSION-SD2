#include "Arduino_H7_Video.h"
#include "lvgl.h"
#include "Arduino_GigaDisplayTouch.h"
#include <AccelStepper.h>

// Stepper Motor Pins (Matches CNC Shield)
#define Z_STEP_PIN 4
#define Z_DIR_PIN 7
#define STEPPER_ENABLE 8

// Steps per mm conversion (adjust based on your hardware)
#define STEPS_PER_MM 100

Arduino_H7_Video Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch TouchDetector;

lv_obj_t *main_screen, *menu_screen, *manual_screen;
lv_obj_t *preset1_screen, *preset2_screen;
lv_obj_t *height_label;

static int height = 0; // Current height in mm

// Create an instance of AccelStepper in DRIVER mode
AccelStepper stepper(AccelStepper::DRIVER, Z_STEP_PIN, Z_DIR_PIN);

// Function to move stepper motor by a given number of steps in a specified direction using acceleration
void moveStepper(int steps, bool direction) {
    // Calculate the target relative to current position.
    // If direction is HIGH, add steps; if LOW, subtract steps.
    int targetSteps = (direction ? steps : -steps);
    stepper.moveTo(stepper.currentPosition() + targetSteps);
}

// Function to create a button with a label
static lv_obj_t *create_button(lv_obj_t *parent, const char *text, lv_event_cb_t event_cb) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 300, 70);
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

// Event callbacks
static void return_menu_event_cb(lv_event_t *e) { 
    lv_scr_load(menu_screen); 
}

static void preset1_event_cb(lv_event_t *e) {
    // Move to 5mm above media: 5mm * STEPS_PER_MM steps
    moveStepper(0.05 * STEPS_PER_MM, HIGH);  // 0.01 is exactly 1 step while full-stepping.
    height = 5;
    lv_label_set_text_fmt(height_label, "Height: %d mm", height);
}

static void preset2_event_cb(lv_event_t *e) {
    // Move to 10mm above media: 10mm * STEPS_PER_MM steps
    moveStepper(0.1 * STEPS_PER_MM, HIGH);  // 0.01 is exactly 1 step while full-stepping.
    height = 10;
    lv_label_set_text_fmt(height_label, "Height: %d mm", height);
}

static void increment_event_cb(lv_event_t *e) {
    // Move up by 1mm
    moveStepper(0.01 * STEPS_PER_MM, HIGH);  // 0.01 is exactly 1 step while full-stepping.
    height++;
    lv_label_set_text_fmt(height_label, "Height: %d mm", height);
}

static void decrement_event_cb(lv_event_t *e) {
    if (height > 0) {
        // Move down by 1mm
        moveStepper(.01 * STEPS_PER_MM, LOW);  // 0.01 is exactly 1 step while full-stepping.
        height--;
        lv_label_set_text_fmt(height_label, "Height: %d mm", height);
    }
}

static void manual_adjustment_event_cb(lv_event_t *e) { 
    lv_scr_load(manual_screen); 
}

void setup() {
    Serial.begin(115200);
    
    // Initialize Stepper Motor Control Pins
    pinMode(Z_STEP_PIN, OUTPUT);
    pinMode(Z_DIR_PIN, OUTPUT);
    pinMode(STEPPER_ENABLE, OUTPUT);
    digitalWrite(STEPPER_ENABLE, LOW);  // Enable stepper motor (LOW = enabled)

    // Initialize Display and Touch
    Display.begin();
    TouchDetector.begin();

    // Configure AccelStepper for smoother, quieter motion
    stepper.setMaxSpeed(1000);      // Maximum steps per second (adjust as needed)
    stepper.setAcceleration(500);  // Steps per second^2 (adjust as needed)

    // Create Menu Screen and Buttons
    menu_screen = lv_obj_create(NULL);
    lv_obj_t *btn1 = create_button(menu_screen, "Preset 1 (5mm)", preset1_event_cb);
    lv_obj_align(btn1, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *btn2 = create_button(menu_screen, "Preset 2 (10mm)", preset2_event_cb);
    lv_obj_align_to(btn2, btn1, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

    lv_obj_t *btn3 = create_button(menu_screen, "Manual Adjustment", manual_adjustment_event_cb);
    lv_obj_align_to(btn3, btn2, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

    // Create Manual Adjustment Screen
    manual_screen = lv_obj_create(NULL);
    height_label = lv_label_create(manual_screen);
    lv_label_set_text(height_label, "Height: 0 mm");
    lv_obj_align(height_label, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *inc_btn = create_button(manual_screen, "+ Increment", increment_event_cb);
    lv_obj_align(inc_btn, LV_ALIGN_CENTER, -100, 0);

    lv_obj_t *dec_btn = create_button(manual_screen, "- Decrement", decrement_event_cb);
    lv_obj_align(dec_btn, LV_ALIGN_CENTER, 100, 0);

    create_button(manual_screen, "Return to Menu", return_menu_event_cb);
    
    lv_scr_load(menu_screen);
}

void loop() {
    lv_timer_handler();
    // Continuously run the stepper (this nonblocking call handles acceleration)
    stepper.run();
    delay(5);  // Prevent CPU overload
}

