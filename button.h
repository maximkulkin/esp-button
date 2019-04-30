#pragma once

typedef enum {
    button_active_low = 0,
    button_active_high = 1,
} button_active_level_t;

typedef struct {
    button_active_level_t active_level;

    // times in milliseconds
    uint16_t debounce_time;

    uint16_t long_press_time;
    uint16_t double_press_time;
} button_config_t;

typedef enum {
    button_event_single_press,
    button_event_double_press,
    button_event_long_press,
} button_event_t;

typedef void (*button_callback_fn)(button_event_t event, void* context);

#define BUTTON_CONFIG(...) \
  (button_config_t) { \
    .active_level = button_active_low, \
    .debounce_time = 20, \
    .long_press_time = 1000, \
    __VA_ARGS__ \
  }

int button_create(uint8_t gpio_num,
                  button_config_t config,
                  button_callback_fn callback,
                  void* context);

void button_destroy(uint8_t gpio_num);
