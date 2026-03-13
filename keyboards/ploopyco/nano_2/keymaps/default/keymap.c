/* Copyright 2021 Colin Lam (Ploopy Corporation)
 * Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 * Copyright 2019 Sunjun Kim
 * Copyright 2019 Hiroyuki Okada
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */
#include QMK_KEYBOARD_H

enum custom_keycodes {
    CST_BTN = SAFE_RANGE
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT( CST_BTN ),
};

#define DELTA_X_THRESHOLD 60
#define DELTA_Y_THRESHOLD 15

// State tracking variables
static int8_t delta_x               = 0;
static int8_t delta_y               = 0;
static bool is_btn_held             = false;
static uint16_t btn_hold_timer      = 0;
static bool has_moved_while_held    = false;
static bool scroll_mode_active      = false;
static bool click_registered        = false;

// 1. Handle Press and Release
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case CST_BTN:
            if (record->event.pressed) {
                // If scroll mode is on, a click turns it off.
                if (scroll_mode_active) {
                    scroll_mode_active = false;
                    return false;
                }

                // Start tracking the hold. DO NOT click yet.
                is_btn_held = true;
                btn_hold_timer = timer_read();
                has_moved_while_held = false;
                click_registered = false;
            } else {
                // Button Released
                is_btn_held = false;

                if (click_registered) {
                    // We were dragging, so release the LMB
                    unregister_code(MS_BTN1);
                    click_registered = false;
                } else if (!has_moved_while_held && !scroll_mode_active && timer_elapsed(btn_hold_timer) < 2000) {
                    // It was a short tap (< 2s) with no movement. Fire a quick click!
                    tap_code(MS_BTN1);
                }
            }
            return false;
    }
    return true;
}

// 2. Monitor for the 2-second hold (Scroll Mode trigger)
void matrix_scan_user(void) {
    if (is_btn_held && !has_moved_while_held && !click_registered && !scroll_mode_active) {
        if (timer_elapsed(btn_hold_timer) > 2000) {
            // Reached 2 seconds without moving. Enter scroll mode.
            scroll_mode_active = true;
            // No need to unregister a click, because we never sent one!
        }
    }
}

// 3. Monitor for Trackball Movement (Drag trigger & Scrolling)
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    // If the button is held and we aren't scrolling, check for movement to start a drag
    if (is_btn_held && !scroll_mode_active) {
        if (mouse_report.x != 0 || mouse_report.y != 0) {
            if (!has_moved_while_held) {
                // This is the moment the ball starts moving. Start the drag!
                has_moved_while_held = true;
                register_code(MS_BTN1); // Hold down LMB
                click_registered = true;
            }
        }
    }

    // If scroll mode is toggled on, convert X/Y movement to scrolling
    if (scroll_mode_active) {
        delta_x += mouse_report.x;
        delta_y += mouse_report.y;

        if (delta_x > DELTA_X_THRESHOLD) {
            mouse_report.h = -1;
            delta_x        = 0;
        } else if (delta_x < -DELTA_X_THRESHOLD) {
            mouse_report.h = 1;
            delta_x        = 0;
        }

        if (delta_y > DELTA_Y_THRESHOLD) {
            mouse_report.v = 1;
            delta_y        = 0;
        } else if (delta_y < -DELTA_Y_THRESHOLD) {
            mouse_report.v = -1;
            delta_y        = 0;
        }

        // Zero out normal pointer movement
        mouse_report.x = 0;
        mouse_report.y = 0;
    }
    return mouse_report;
}
