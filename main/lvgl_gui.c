#include "lvgl_gui.h"

#define SYMBOL_CLOCK "\xEF\x80\x97"

static lv_group_t *group_root = NULL;
static lv_obj_t *dummy_obj_event_handler = NULL;
static lv_obj_t *label_battery_group_root = NULL;
static lv_obj_t *label_alias_group_1 = NULL;
static lv_obj_t *bar_time_progress_group1 = NULL;
static lv_obj_t *label_code_group_1 = NULL;
static lv_obj_t *label_time_group_2 = NULL;
static lv_obj_t *label_sync_time_group_3 = NULL;
static lv_obj_t *label_sync_time_spinner_group_3 = NULL;
static lv_obj_t *label_setting_group_4 = NULL;
static lv_obj_t *label_ap_name_group_4_1 = NULL;
static lv_obj_t *label_ap_pass_group_4_1 = NULL;
static lv_obj_t *image_qr_code_group_4_2 = NULL;
static lv_obj_t *label_ip_addr_group_4_2 = NULL;

static lv_obj_t *scr1 = NULL;
static lv_obj_t *scr2 = NULL;
static lv_obj_t *scr3[2] = {NULL, NULL};
static lv_obj_t *scr4[3] = {NULL, NULL, NULL};

lv_indev_t *my_indev = NULL;

lv_task_t *task_time_update = NULL;
lv_task_t *task_key_update_task = NULL;

static int menu_id = 0;
static int key_id = 0;
// true means move up and false means move down
static bool move_direction = false;

static int scr3_submenu_id = 0;
static int scr4_submenu_id = 0;

// credits to this function: https://codereview.stackexchange.com/a/29200
static void random_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijkmnopqrstuvwxyzABCDEFGHJKZ";
    if (size)
    {
        --size;
        for (size_t n = 0; n < size; n++)
        {
            int key = rand() % (int)(sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
}

static void action_menu_page()
{
    int key_count = 0;
    char passkey[64] = "";

    switch (menu_id)
    {
    case 0:
        key_count = read_totp_key_count();
        key_id = key_count >= 0 ? ((key_id + 1) % key_count + key_count) % key_count : 0;
        lv_task_ready(task_key_update_task);
        break;
    case 2:
        post_gui_events(START_SYNC_TIME, NULL, sizeof(NULL));
        break;

    case 3:
        strncpy(passkey, "pass123456", 64);
        // random_string(passkey, 10);
        lv_label_set_text_fmt(label_ap_pass_group_4_1, "\t" LV_SYMBOL_WIFI " key\n%s", passkey);

        post_gui_events(START_ACCESS_POINT, (void *)passkey, (strlen(passkey) + 1) * sizeof(passkey));
        post_gui_events(START_CONFIG_SERVER, NULL, sizeof(NULL));
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
        lv_scr_load_anim(scr3[1], anim_direction, 100, 100, false);
        break;

    case 3:
        lv_scr_load_anim(scr4[scr4_submenu_id], anim_direction, 100, 100, false);
        break;
    }
}

static void lv_time_update_task(lv_task_t *task)
{
    char time[50];
    struct tm time_now;
    time_t temp = rtc_ext_get_time();
    time_now = *localtime(&temp);

    snprintf(time, 50, "%02d:%02d:%02d", time_now.tm_hour, time_now.tm_min, time_now.tm_sec);
    lv_label_set_text(label_time_group_2, time);
    lv_obj_align(label_time_group_2, NULL, LV_ALIGN_CENTER, 0, 0);
}

static void lv_key_update_task(lv_task_t *task)
{
    totp_key_creds key_temp;
    int key_count = read_totp_key_count();
    key_id = key_id >= key_count && key_count != 0 ? key_count - 1 : key_id;
    char result[10];

    if (read_totp_key_count() > 0 && read_totp_key(key_id, &key_temp) == ESP_OK)
    {
        totp_init(MBEDTLS_MD_SHA1);
        totp_generate(key_temp.key, ((unsigned)time(NULL)) / 30, 6, result);
        totp_free();

        lv_label_set_text(label_alias_group_1, key_temp.alias);
        lv_label_set_text(label_code_group_1, result);

        lv_obj_align(label_alias_group_1, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
        lv_obj_align(label_code_group_1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

        float bar_val = 100.0 * ((unsigned)time(NULL) % 30) / 30.0;
        lv_obj_set_hidden(bar_time_progress_group1, false);
        lv_bar_set_value(bar_time_progress_group1, (int)ceil(bar_val), LV_ANIM_ON);
    }
    else
    {
        lv_label_set_text(label_alias_group_1, "No keys\nenrolled");
        lv_label_set_text(label_code_group_1, " ");
        lv_obj_set_hidden(bar_time_progress_group1, true);
        lv_obj_align(label_alias_group_1, NULL, LV_ALIGN_CENTER, 0, 0);
    }
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

static void lv_tick_task(void *arg)
{
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void lvgl_gui_init_drivers()
{
    lv_init();
    lvgl_driver_init();
    static lv_color_t buf1[DISP_BUF_SIZE];
    static lv_color_t *buf2 = NULL;
    static lv_disp_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE * 8;

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
    scr1 = lv_obj_create(NULL, NULL);
    scr2 = lv_obj_create(NULL, NULL);
    scr3[0] = lv_obj_create(NULL, NULL);
    scr3[1] = lv_obj_create(NULL, NULL);
    scr4[0] = lv_obj_create(NULL, NULL);
    scr4[1] = lv_obj_create(NULL, NULL);
    scr4[2] = lv_obj_create(NULL, NULL);

    dummy_obj_event_handler = lv_obj_create(NULL, NULL);
    label_battery_group_root = lv_label_create(scr1, NULL);
    label_alias_group_1 = lv_label_create(scr1, NULL);
    bar_time_progress_group1 = lv_bar_create(scr1, NULL);
    label_code_group_1 = lv_label_create(scr1, NULL);
    label_time_group_2 = lv_label_create(scr2, NULL);
    label_sync_time_group_3 = lv_label_create(scr3[0], NULL);
    label_sync_time_spinner_group_3 = lv_spinner_create(scr3[1], NULL);
    label_setting_group_4 = lv_label_create(scr4[0], NULL);
    label_ap_name_group_4_1 = lv_label_create(scr4[1], NULL);
    label_ap_pass_group_4_1 = lv_label_create(scr4[1], NULL);
    label_ip_addr_group_4_2 = lv_label_create(scr4[2], NULL);
    image_qr_code_group_4_2 = lv_img_create(scr4[2], NULL);

    lv_label_set_text(label_battery_group_root, " ");
    lv_label_set_text(label_alias_group_1, " ");
    lv_label_set_text(label_code_group_1, " ");
    lv_label_set_text(label_time_group_2, "\t\t\t\t" SYMBOL_CLOCK "\nTIME");
    lv_label_set_text(label_sync_time_group_3, "\t\t\t\t" LV_SYMBOL_LOOP "\nSYNC TIME");
    lv_label_set_text(label_setting_group_4, "\t\t\t\t" LV_SYMBOL_SETTINGS "\nSETTINGS");
    lv_label_set_text(label_ap_name_group_4_1, "connect to open-authenticator " LV_SYMBOL_WIFI);
    lv_label_set_text(label_ap_pass_group_4_1, " ");
    lv_label_set_text(label_ip_addr_group_4_2, " ");

    lv_obj_align(label_time_group_2, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(label_sync_time_group_3, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(label_setting_group_4, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(label_ap_name_group_4_1, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
    lv_obj_align(label_ap_pass_group_4_1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    // add group_4_2 elements too

    lv_obj_set_size(bar_time_progress_group1, 128, 10);
    lv_bar_set_type(bar_time_progress_group1, LV_BAR_TYPE_SYMMETRICAL);
    lv_obj_align(bar_time_progress_group1, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_obj_set_size(label_sync_time_spinner_group_3, 50, 50);
    lv_spinner_set_type(label_sync_time_spinner_group_3, LV_SPINNER_TYPE_FILLSPIN_ARC);
    lv_obj_align(label_sync_time_spinner_group_3, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_label_set_long_mode(label_ap_name_group_4_1, LV_LABEL_LONG_SROLL_CIRC);
    lv_obj_set_width(label_ap_name_group_4_1, 128);

    group_root = lv_group_create();

    lv_group_add_obj(group_root, dummy_obj_event_handler);
    lv_group_add_obj(group_root, label_alias_group_1);
    lv_group_add_obj(group_root, bar_time_progress_group1);
    lv_group_add_obj(group_root, label_code_group_1);
    lv_group_add_obj(group_root, label_time_group_2);
    lv_group_add_obj(group_root, label_sync_time_group_3);
    lv_group_add_obj(group_root, label_sync_time_spinner_group_3);
    lv_group_add_obj(group_root, label_setting_group_4);
    lv_group_add_obj(group_root, label_ap_name_group_4_1);
    lv_group_add_obj(group_root, label_ap_pass_group_4_1);
    lv_group_add_obj(group_root, label_ip_addr_group_4_2);
    lv_group_add_obj(group_root, image_qr_code_group_4_2);

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

    task_time_update = lv_task_create(lv_time_update_task, 1000, LV_TASK_PRIO_MID, NULL);
    task_key_update_task = lv_task_create(lv_key_update_task, 1000, LV_TASK_PRIO_HIGHEST, NULL);
    lv_task_ready(task_time_update);
    lv_task_ready(task_key_update_task);

    while (1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}