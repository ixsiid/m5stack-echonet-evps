#pragma once

#undef min
#undef max
#include <functional>

#include <driver/gpio.h>
#include <esp_timer.h>



typedef struct {
	gpio_num_t pin;

	bool released;
	bool pressed;

	int64_t up_last_changed;
	int64_t down_last_changed;

	void * next;
} InterruptManager_t;

// typedef void (*ButtonCallback)(uint8_t, void* args);
typedef std::function<void(uint8_t)> ButtonCallback;

class Button {
    private:
	static const int64_t dtime_threshould = 10 * 1000; // 10msec
	static bool isr;

	InterruptManager_t * pins;

    public:
	Button(const uint8_t * pins, size_t count);

	void check(ButtonCallback pressed, ButtonCallback released);
};
