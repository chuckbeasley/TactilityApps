#include "Application.h"
#include "PixelBuffer.h"
#include "esp_log.h"

#include <app/event.h>
#include <tactility/check.h>

#include <Tactility/kernel/Kernel.h>

constexpr auto TAG = "Application";

static bool isTouched(TouchDriver* touch) {
    uint16_t x, y, strength;
    uint8_t pointCount = 0;
    return touch->getTouchedPoints(&x, &y, &strength, &pointCount, 1);
}

void createRgbRow(PixelBuffer& buffer) {
    uint8_t offset = buffer.getPixelWidth() / 3;
    for (int i = 0; i < buffer.getPixelWidth(); ++i) {
        if (i < offset) {
            buffer.setPixel(i, 0, 255, 0, 0);
        } else if (i < offset * 2) {
            buffer.setPixel(i, 0, 0, 255, 0);
        } else {
            buffer.setPixel(i, 0, 0, 0, 255);
        }
    }
}

void createRgbFadingRow(PixelBuffer& buffer) {
    uint8_t stroke = buffer.getPixelWidth() / 3;
    for (int i = 0; i < buffer.getPixelWidth(); ++i) {
        if (i < stroke) {
            auto color = i * 255 / stroke;
            buffer.setPixel(i, 0, color, 0, 0);
        } else if (i < stroke * 2) {
            auto color = (i - stroke) * 255 / stroke;
            buffer.setPixel(i, 0, 0, color, 0);
        } else {
            auto color = (i - (2*stroke)) * 255 / stroke;
            buffer.setPixel(i, 0, 0, 0, color);
        }
    }
}

void runApplication(DisplayDriver* display, TouchDriver* touch) {
    // Single row buffers
    PixelBuffer line_clear_buffer(display->getWidth(), 1, display->getColorFormat());
    line_clear_buffer.clear();
    PixelBuffer line_buffer(display->getWidth(), 1, display->getColorFormat());
    line_buffer.clear();

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
        // Draw row by row
        // This is placed in a loop to test the SPI locking mechanismss
        for (int i = 0; i < display->getHeight(); i++) {

            if (i == 0) {
                createRgbRow(line_buffer);
            } else if (i == display->getHeight() / 2) {
                createRgbFadingRow(line_buffer);
            }

            display->lock();
            display->drawBitmap(0, i, display->getWidth(), i + 1, line_buffer.getData());
            display->unlock();
        }

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

        // Give other tasks space to breathe
        // SPI displays would otherwise time out SPI SD card access
        tt::kernel::delayTicks(1);
    } while (!isTouched(touch));

    check(app_event_unsubscribe(&sub) == ERROR_NONE);
    task_event_group_destruct(&event_group);
}

