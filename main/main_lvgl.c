#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <ds3231.h>
#include "ntp.h"
#include "totp.h"
#include "lvgl_gui.h"
#include "wifi_handler_station.h"

#include "oa_power.h"
#include "oa_pin_defs.h"
#include "oa_battery.h"
#include "oa_switches.h"

#include "sdkconfig.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "lvgl.h"
#include "lvgl_helpers.h"

static void wifi_ntp_task(void *arg);

void app_main()
{
    init_load_switch();
    init_switches();
    activate_load_switch();

    config_adc1();
    characterize_adc1();

    xTaskCreatePinnedToCore(wifi_ntp_task, "ntp", 4096, NULL, 0, NULL, 0);
    xTaskCreatePinnedToCore(lvgl_gui_task, "gui", 4096, NULL, 0, NULL, 1);
    // lvgl_gui_task();
}

static void wifi_ntp_task(void *arg)
{
    while (1)
    {
        while (1)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(start_wifi_station("{\"c\":1,\"s\":[\"sdfsdfdsf\"],\"p\":[\"sdfdsfs\"]}"));
            ntp_get_time();
            stop_wifi_station();

            vTaskDelay(864000 / portTICK_PERIOD_MS);
        }
        vTaskDelay(3600 / portTICK_PERIOD_MS);
    }
}
