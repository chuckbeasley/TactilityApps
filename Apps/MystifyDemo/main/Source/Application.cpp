#include "Application.h"
#include "MystifyDemo.h"
#include "PixelBuffer.h"
#include "esp_log.h"

#include <app/event.h>
#include <tactility/check.h>

#include <Tactility/kernel/Kernel.h>

constexpr auto TAG = "Application";
constexpr int MYSTIFY_FRAME_DELAY_MS = 50;  // ~20 FPS for smooth animation

static bool isTouched(TouchDriver* touch) {
    uint16_t x, y, strength;
    uint8_t pointCount = 0;
    return touch->getTouchedPoints(&x, &y, &strength, &pointCount, 1);
}

void runApplication(DisplayDriver* display, TouchDriver* touch) {
    // Run the Mystify screensaver demo
    MystifyDemo mystify;
    if (!mystify.init(display)) {
        ESP_LOGE(TAG, "Failed to initialize MystifyDemo");
        return;
    }

    ESP_LOGI(TAG, "Starting Mystify demo - touch to exit");

    // This app borrows the display directly (it stops the LVGL module), so instead of a normal
    // window event loop it must poll for APP_EVENT_CLOSE itself - otherwise the app manager can
    // never stop it (app_scheduler_stop waits for the task to acknowledge the close event), and
    // a borrow-left LVGL-stopped state would break every subsequent app load.
    struct TaskEventGroup event_group {};
    task_event_group_construct(&event_group);
    struct AppEventSubscription sub {};
    check(app_event_subscribe(&sub, &event_group) == ERROR_NONE);

    bool should_close = false;
    do {
        mystify.update();

        // Drain any app-manager close request (non-blocking, once per frame).
        struct AppEvent event {};
        while (app_event_poll(&sub, &event) == ERROR_NONE) {
            if (event.type == APP_EVENT_CLOSE) {
                should_close = true;
            }
        }
        if (should_close) {
            break;
        }

        // Frame rate limiter - ~20 FPS for smooth animation
        tt::kernel::delayTicks(tt::kernel::millisToTicks(MYSTIFY_FRAME_DELAY_MS));
    } while (!isTouched(touch));

    check(app_event_unsubscribe(&sub) == ERROR_NONE);
    task_event_group_destruct(&event_group);

    ESP_LOGI(TAG, "Mystify demo ended");
}

