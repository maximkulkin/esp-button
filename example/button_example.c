#include <stdio.h>
#include <stdlib.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>

#include <button.h>

#ifndef BUTTON_GPIO
#error BUTTON_GPIO just be defined
#endif

void idle_task(void* arg) {
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void button_callback(button_event_t event, void* context) {
    switch (event) {
        case button_event_single_press:
            printf("single press\n");
            break;
        case button_event_double_press:
            printf("double press\n");
            break;
        case button_event_long_press:
            printf("long press\n");
            break;
    }
}


void user_init(void) {
    uart_set_baud(0, 115200);

    button_config_t button_config = BUTTON_CONFIG(
        .active_level=button_active_low,
    );

    int r = button_create(BUTTON_GPIO, button_config, button_callback, NULL);
    if (r) {
        printf("Failed to initalize button (code %d)\n", r);
    }

    printf("Button example\n");

    xTaskCreate(idle_task, "Idle task", 256, NULL, tskIDLE_PRIORITY, NULL);
}
