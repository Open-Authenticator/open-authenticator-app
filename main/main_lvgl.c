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
#include "gui_event_handler.h"

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

    ESP_ERROR_CHECK(init_spiffs());
    // ESP_LOGI("main", "%s", read_wifi_ap_from_spiffs());
    // ESP_LOGI("main", "%s", read_wifi_creds());
    // ESP_LOGI("main", "%d", remove_wifi_ap_from_spiffs("D-Link"));
    // ESP_LOGI("main", "%s", read_wifi_ap_from_spiffs());
    // ESP_LOGI("main", "%s", read_wifi_creds());
    // ESP_LOGI("main", "%d", write_wifi_ap_pass_to_spiffs("D-Link", "passkey123"));
    // ESP_LOGI("main", "%s", read_wifi_ap_from_spiffs());
    // ESP_LOGI("main", "%s", read_wifi_creds());
    start_gui_event_handler();
    xTaskCreatePinnedToCore(wifi_ntp_task, "ntp", 4096, NULL, 0, NULL, 0);
    xTaskCreatePinnedToCore(lvgl_gui_task, "gui", 4096*3, NULL, 0, NULL, 1);
}

static void wifi_ntp_task(void *arg)
{
    while (1)
    {
        while (1)
        {
            post_gui_events(START_SYNC_TIME, NULL, sizeof(NULL));
            vTaskDelay(864000 / portTICK_PERIOD_MS);
        }
        vTaskDelay(3600 / portTICK_PERIOD_MS);
    }
}
