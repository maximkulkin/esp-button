#include <stdio.h>
#include <string.h>
#include <etstimer.h>
#include <esp/interrupts.h>
#include <esplibs/libmain.h>
#include "button.h"


typedef struct _button {
    uint8_t gpio_num;
    button_config_t config;
    button_callback_fn callback;
    void* context;

    uint8_t press_count;
    ETSTimer press_timer;
    uint32_t last_press_time;
    uint32_t last_event_time;

    struct _button *next;
} button_t;


button_t *buttons = NULL;


static button_t *button_find_by_gpio(const uint8_t gpio_num) {
    button_t *button = buttons;
    while (button && button->gpio_num != gpio_num)
        button = button->next;

    return button;
}


void button_intr_callback(uint8_t gpio) {
    uint32_t interrupts = _xt_disable_interrupts();

    button_t *button = button_find_by_gpio(gpio);
    if (!button) {
        _xt_restore_interrupts(interrupts);
        return;
    }

    uint32_t now = xTaskGetTickCountFromISR();
    if ((now - button->last_event_time)*portTICK_PERIOD_MS < button->config.debounce_time) {
        // debounce time, ignore events
        _xt_restore_interrupts(interrupts);
        return;
    }

    button->last_event_time = now;
    if (gpio_read(button->gpio_num) == (int)button->config.active_level) {
        button->last_press_time = now;
    } else {
        uint32_t press_duration = (now - button->last_press_time)*portTICK_PERIOD_MS;
        if (button->config.long_press_time &&
            press_duration > button->config.long_press_time)
        {
            button->press_count = 0;

            button->callback(button_event_long_press, button->context);
        } else {
            button->press_count++;
            if (button->press_count > 1) {
                button->press_count = 0;
                sdk_os_timer_disarm(&button->press_timer);

                button->callback(button_event_double_press, button->context);
            } else {
                if (button->config.double_press_time) {
                    sdk_os_timer_arm(&button->press_timer,
                                     button->config.double_press_time, 1);
                } else {
                    button->press_count = 0;
                    button->callback(button_event_single_press, button->context);
                }
            }
        }
    }

    _xt_restore_interrupts(interrupts);
}


void button_timer_callback(void *arg) {
    button_t *button = arg;

    button->press_count = 0;
    sdk_os_timer_disarm(&button->press_timer);

    button->callback(button_event_single_press, button->context);
}


int button_create(const uint8_t gpio_num,
                  button_config_t config,
                  button_callback_fn callback,
                  void* context)
{
    button_t *button = button_find_by_gpio(gpio_num);
    if (button)
        return -1;

    button = malloc(sizeof(button_t));
    memset(button, 0, sizeof(*button));
    button->gpio_num = gpio_num;
    button->config = config;
    button->callback = callback;
    button->context = context;

    button->next = buttons;
    buttons = button;

    bool pullup = (config.active_level == button_active_low);
    gpio_set_pullup(button->gpio_num, pullup, pullup);
    gpio_set_interrupt(button->gpio_num, GPIO_INTTYPE_EDGE_ANY, button_intr_callback);

    sdk_os_timer_disarm(&button->press_timer);
    sdk_os_timer_setfn(&button->press_timer, button_timer_callback, button);

    return 0;
}


void button_delete(const uint8_t gpio_num) {
    if (!buttons)
        return;

    button_t *button = NULL;
    if (buttons->gpio_num == gpio_num) {
        button = buttons;
        buttons = buttons->next;
    } else {
        button_t *b = buttons;
        while (b->next) {
            if (b->next->gpio_num == gpio_num) {
                button = b->next;
                b->next = b->next->next;
                break;
            }
        }
    }

    if (button) {
        sdk_os_timer_disarm(&button->press_timer);
        gpio_set_interrupt(button->gpio_num, GPIO_INTTYPE_EDGE_ANY, NULL);
    }
}

