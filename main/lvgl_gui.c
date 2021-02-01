#include "lvgl_gui.h"

static lv_group_t *group_1 = NULL;
static lv_group_t *group_2 = NULL;
static lv_group_t *group_3 = NULL;
static lv_group_t *group_4 = NULL;
static lv_group_t *group_4_1 = NULL;
static lv_group_t *group_4_2 = NULL;

static lv_obj_t *label_battery_group_root = NULL;
static lv_obj_t *label_alias_group_1 = NULL;
static lv_obj_t *label_code_group_1 = NULL;
static lv_obj_t *label_time_group_2 = NULL;
static lv_obj_t *label_sync_time_group_3 = NULL;
static lv_obj_t *label_ap_name_group_4_1 = NULL;
static lv_obj_t *label_ap_pass_group_4_1 = NULL;
// static lv_img_dsc_t *image_qr_code_group_4_2 = NULL;
static lv_obj_t *label_ip_addr_group_4_2 = NULL;

static lv_obj_t *scr = NULL;

lv_indev_t *my_indev = NULL;

static void lv_tick_task(void *arg)
{
    lv_task_handler();
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

int read_switch_id()
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

bool encoder_with_switches(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    int key_id = read_switch_id();

    if (key_id > 0)
    {
        data->key = key_id;
        data->state = LV_INDEV_STATE_PR;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

    return false;
}

static void switch_event_handler_cb(lv_obj_t *obj, lv_event_t event)
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

static void lvgl_gui_init_drivers()
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

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_with_switches;
    my_indev = lv_indev_drv_register(&indev_drv);
}

static void lvgl_gui_init_obj()
{
    scr = lv_disp_get_scr_act(NULL);

    label_battery_group_root = lv_label_create(scr, NULL);
    label_alias_group_1 = lv_label_create(scr, NULL);
    label_code_group_1 = lv_label_create(scr, NULL);
    label_time_group_2 = lv_label_create(scr, NULL);
    label_sync_time_group_3 = lv_label_create(scr, NULL);
    label_ap_name_group_4_1 = lv_label_create(scr, NULL);
    label_ap_pass_group_4_1 = lv_label_create(scr, NULL);
    label_ip_addr_group_4_2 = lv_label_create(scr, NULL);

    lv_label_set_text(label_battery_group_root, " ");
    lv_label_set_text(label_alias_group_1, " ");
    lv_label_set_text(label_code_group_1, " ");
    lv_label_set_text(label_time_group_2, " ");
    lv_label_set_text(label_sync_time_group_3, " ");
    lv_label_set_text(label_ap_name_group_4_1, " ");
    lv_label_set_text(label_ap_pass_group_4_1, " ");
    lv_label_set_text(label_ip_addr_group_4_2, " ");

    group_1 = lv_group_create();
    group_2 = lv_group_create();
    group_3 = lv_group_create();
    group_4 = lv_group_create();
    group_4_1 = lv_group_create();
    group_4_2 = lv_group_create();

    lv_obj_set_event_cb(label_alias_group_1, switch_event_handler_cb);
    lv_obj_set_event_cb(label_code_group_1, switch_event_handler_cb);
    lv_group_add_obj(group_1, label_alias_group_1);
    lv_group_add_obj(group_1, label_code_group_1);

    lv_indev_set_group(my_indev, group_1);
    lv_group_focus_obj(label_alias_group_1);
}

void lvgl_gui_task()
{
    struct tm time_now;
    rtc_ext_init(RTC_SDA, RTC_SCL);

    char *key = "JBSWY3DPEHPK3PXP";
    char res[10];

    struct timeval tm = {.tv_sec = rtc_ext_get_time()};
    settimeofday(&tm, NULL);

    lvgl_gui_init_drivers();
    lvgl_gui_init_obj();

    while (1)
    {
        char time_[50];
        time_t temp = rtc_ext_get_time();
        time_now = *localtime(&temp);

        snprintf(time_, 50, "%02d:%02d:%02d", time_now.tm_hour, time_now.tm_min, time_now.tm_sec);
        totp_init(MBEDTLS_MD_SHA1);
        totp_generate(key, ((unsigned)time(NULL)) / 30, 6, res);
        totp_free();

        lv_label_set_text(label_alias_group_1, time_);
        lv_label_set_text(label_code_group_1, res);

        lv_obj_align(label_alias_group_1, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
        lv_obj_align(label_code_group_1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}