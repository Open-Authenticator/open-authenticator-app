#include "lvgl_gui.h"

static lv_group_t *group_root = NULL;

static lv_obj_t *dummy_obj_event_handler = NULL;
static lv_obj_t *label_battery_group_root = NULL;
static lv_obj_t *label_alias_group_1 = NULL;
static lv_obj_t *label_code_group_1 = NULL;
static lv_obj_t *label_time_group_2 = NULL;
static lv_obj_t *label_sync_time_group_3 = NULL;
static lv_obj_t *label_ap_name_group_4_1 = NULL;
static lv_obj_t *label_ap_pass_group_4_1 = NULL;
// static lv_img_dsc_t *image_qr_code_group_4_2 = NULL;
static lv_obj_t *label_ip_addr_group_4_2 = NULL;

static lv_obj_t *scr1 = NULL;
static lv_obj_t *scr2 = NULL;
static lv_obj_t *scr3 = NULL;
static lv_obj_t *scr4 = NULL;

lv_indev_t *my_indev = NULL;

static int menu_id = 0;
// true means move up and false means move down
static bool move_direction = false;

void action_connect_to_wifi()
{    
    while(start_wifi_station("{\"c\":1,\"s\":[\"Dsfsdf\"],\"p\":[\"dsfsdfsdf\"]}") == WIFI_ERR_ALREADY_RUNNING)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ntp_get_time();
    stop_wifi_station();

    vTaskDelete(NULL);
}

static void action_menu_page()
{
    switch(menu_id)
    {
        case 2:
            lv_label_set_text(label_sync_time_group_3, LV_SYMBOL_LOOP);
            lv_obj_align(label_sync_time_group_3, NULL, LV_ALIGN_CENTER, 0, 0);
            xTaskCreatePinnedToCore(action_connect_to_wifi, "ntp", 4096, NULL, 0, NULL, 0);
            break;
    }
}

static void set_menu_page()
{
    lv_scr_load_anim_t anim_direction = move_direction ? LV_SCR_LOAD_ANIM_OVER_TOP : LV_SCR_LOAD_ANIM_OVER_BOTTOM;

    switch (menu_id)
    {
        case 0:
            lv_scr_load_anim(scr1, anim_direction, 100, 100, false);
            break;

        case 1:
            lv_scr_load_anim(scr2, anim_direction, 100, 100, false);
            break;

        case 2:
            lv_scr_load_anim(scr3, anim_direction, 100, 100, false);
            lv_label_set_text(label_sync_time_group_3, "\t\t\t\t" LV_SYMBOL_LOOP "\nSYNC TIME");
            lv_obj_align(label_sync_time_group_3, NULL, LV_ALIGN_CENTER, 0, 0);
            break;

        case 3:
            lv_scr_load_anim(scr4, anim_direction, 100, 100, false);
            break;
    }
}

static void lv_tick_task(void *arg)
{
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void lv_time_update_task(void *arg)
{
    char time[50];
    struct tm time_now;
    time_t temp = rtc_ext_get_time();
    time_now = *localtime(&temp);

    snprintf(time, 50, "%02d:%02d:%02d", time_now.tm_hour, time_now.tm_min, time_now.tm_sec);
    lv_label_set_text(label_time_group_2, time);
    lv_obj_align(label_time_group_2, NULL, LV_ALIGN_CENTER, 0, 0);
}

static void lv_key_update_task(void *arg)
{
    char *key = "JBSWY3DPEHPK3PXP";
    char result[10];

    totp_init(MBEDTLS_MD_SHA1);
    totp_generate(key, ((unsigned)time(NULL)) / 30, 6, result);
    totp_free();

    lv_label_set_text(label_alias_group_1, "Google");
    lv_label_set_text(label_code_group_1, result);

    lv_obj_align(label_alias_group_1, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
    lv_obj_align(label_code_group_1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
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
    if (event == LV_EVENT_PRESSED)
    {
        ESP_LOGI("button_cb", "Clicked button - enter");
        action_menu_page();
    }
    else if (event == LV_EVENT_KEY)
    {
        key_id = (uint32_t *)lv_event_get_data();
        ESP_LOGI("button_cb", "Clicked button - %u", *key_id);

        if (*key_id == LV_KEY_UP)
        {
            menu_id = ((menu_id - 1) % 4 + 4) % 4;
            move_direction = true;
        }
        else if (*key_id == LV_KEY_DOWN)
        {
            menu_id = ((menu_id + 1) % 4 + 4) % 4;
            move_direction = false;
        }
        set_menu_page();
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
        .name = "periodic_gui"
    };
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
    scr1 = lv_obj_create(NULL, NULL);
    scr2 = lv_obj_create(NULL, NULL);
    scr3 = lv_obj_create(NULL, NULL);
    scr4 = lv_obj_create(NULL, NULL);

    dummy_obj_event_handler = lv_obj_create(NULL, NULL);
    label_battery_group_root = lv_label_create(scr1, NULL);
    label_alias_group_1 = lv_label_create(scr1, NULL);
    label_code_group_1 = lv_label_create(scr1, NULL);
    label_time_group_2 = lv_label_create(scr2, NULL);
    label_sync_time_group_3 = lv_label_create(scr3, NULL);
    label_ap_name_group_4_1 = lv_label_create(scr4, NULL);
    label_ap_pass_group_4_1 = lv_label_create(scr4, NULL);
    label_ip_addr_group_4_2 = lv_label_create(scr4, NULL);

    lv_label_set_text(label_battery_group_root, " ");
    lv_label_set_text(label_alias_group_1, " ");
    lv_label_set_text(label_code_group_1, " ");
    lv_label_set_text(label_time_group_2, " ");
    lv_label_set_text(label_sync_time_group_3, " ");
    lv_label_set_text(label_ap_name_group_4_1, " ");
    lv_label_set_text(label_ap_pass_group_4_1, " ");
    lv_label_set_text(label_ip_addr_group_4_2, " ");

    group_root = lv_group_create();

    lv_group_add_obj(group_root, dummy_obj_event_handler);
    lv_group_add_obj(group_root, label_alias_group_1);
    lv_group_add_obj(group_root, label_code_group_1);
    lv_group_add_obj(group_root, label_time_group_2);
    lv_group_add_obj(group_root, label_sync_time_group_3);
    lv_group_add_obj(group_root, label_ap_name_group_4_1);
    lv_group_add_obj(group_root, label_ap_pass_group_4_1);
    lv_group_add_obj(group_root, label_ip_addr_group_4_2);
    
    lv_obj_set_event_cb(dummy_obj_event_handler, switch_event_handler_cb);

    lv_indev_set_group(my_indev, group_root);
    lv_scr_load_anim(scr1, LV_SCR_LOAD_ANIM_FADE_ON, 100, 100, false);
}

void lvgl_gui_task()
{
    rtc_ext_init(RTC_SDA, RTC_SCL);

    struct timeval tm = {.tv_sec = rtc_ext_get_time()};
    settimeofday(&tm, NULL);

    lvgl_gui_init_drivers();
    lvgl_gui_init_obj();

    lv_task_t *task_time_update = lv_task_create(lv_time_update_task, 500, LV_TASK_PRIO_HIGHEST, NULL);
    lv_task_t *task_key_update_task = lv_task_create(lv_key_update_task, 10000, LV_TASK_PRIO_HIGHEST, NULL);
    lv_task_ready(task_time_update);
    lv_task_ready(task_key_update_task);

    while (1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}