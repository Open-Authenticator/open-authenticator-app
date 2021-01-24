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

#define TAG "demo"
#define LV_TICK_PERIOD_MS 1

static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
static void wifi_ntp_task(void *arg);

void app_main()
{

    init_load_switch();
    init_switches();
    activate_load_switch();

    config_adc1();
    characterize_adc1();

    xTaskCreatePinnedToCore(wifi_ntp_task, "ntp", 4096 * 2, NULL, 0, NULL, 0);

    xTaskCreatePinnedToCore(guiTask, "gui", 4096 * 2, NULL, 0, NULL, 1);
}

int read_key()
{
    if (read_switch(SWITCH_UP))
    {
        return LV_KEY_UP;
    }
    else if (read_switch(SWITCH_DOWN))
    {
        return LV_KEY_DOWN;
    }
    else if (read_switch(SWITCH_SELECT))
    {
        return LV_KEY_ENTER;
    }

    return 0;
}

bool encoder_with_buttons(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    int key_id = read_key();

    if (key_id > 0)
    {
        // ESP_LOGI("button_handler", "pressed key %d", key_id);
        data->key = key_id;
        data->state = LV_INDEV_STATE_PR;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

    return false;
}

static void button_event_handler_cb(lv_obj_t *obj, lv_event_t event)
{
    uint32_t *key_id = NULL;
    switch (event)
    {
        case LV_EVENT_PRESSED:
            ESP_LOGI("button_cb", "Clicked button - enter");
            lv_obj_set_hidden(obj, !lv_obj_get_hidden(obj));
            break;

        case LV_EVENT_KEY:
            key_id = (uint32_t *)lv_event_get_data();
            ESP_LOGI("button_cb", "Clicked button - %u", *key_id);
            break;
    }
}

static void guiTask(void *pvParameter)
{

    lv_init();
    lvgl_driver_init();

    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t *buf2 = NULL;
    static lv_disp_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;

    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_with_buttons;
    lv_indev_t *my_indev = lv_indev_drv_register(&indev_drv);

    /* Get the current screen  */
    lv_obj_t *scr = lv_disp_get_scr_act(NULL);

    /*Create a Label on the currently active screen*/
    lv_obj_t *label1 = lv_label_create(scr, NULL);
    lv_obj_t *label2 = lv_label_create(scr, NULL);
    lv_label_set_text(label1, " ");
    lv_label_set_text(label2, " ");
    lv_group_t *group1 = lv_group_create();
    lv_obj_set_event_cb(label1, button_event_handler_cb);
    lv_obj_set_event_cb(label2, button_event_handler_cb);
    lv_group_add_obj(group1, label1);
    lv_group_add_obj(group1, label2);
    lv_indev_set_group(my_indev, group1);
    lv_group_focus_obj(label1);

    struct tm time_now;
    rtc_ext_init(RTC_SDA, RTC_SCL);

    char *key = "JBSWY3DPEHPK3PXP";
    char res[10];

    struct timeval tm = {.tv_sec = rtc_ext_get_time()};
    settimeofday(&tm, NULL);

    while (1)
    {
        char time_[50];
        time_t temp = rtc_ext_get_time();
        time_now = *localtime(&temp);

        snprintf(time_, 50, "%02d:%02d:%02d", time_now.tm_hour, time_now.tm_min, time_now.tm_sec);
        totp_init(MBEDTLS_MD_SHA1);
        totp_generate(key, ((unsigned)time(NULL)) / 30, 6, res);
        totp_free();

        lv_label_set_text(label1, time_);
        lv_label_set_text(label2, res);

        lv_obj_align(label1, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
        lv_obj_align(label2, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

static void lv_tick_task(void *arg)
{
    lv_task_handler();
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void wifi_ntp_task(void *arg)
{
    while (1)
    {
        while (1)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(start_wifi_station("{\"c\":1,\"s\":[\"D-Li\"],\"p\":[\"fskdhd\"]}"));
            ntp_get_time();
            stop_wifi_station();

            vTaskDelay(86400 / portTICK_PERIOD_MS);
        }
        vTaskDelay(3600 / portTICK_PERIOD_MS);
    }
}
