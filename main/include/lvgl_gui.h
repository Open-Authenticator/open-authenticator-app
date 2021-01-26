#ifndef LVGL_GUI_H
#define LVGL_GUI_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ntp.h"
#include "totp.h"
#include "rtc.h"
#include "sync.h"
#include "wifi_handler_station.h"

#include "oa_power.h"
#include "oa_pin_defs.h"
#include "oa_battery.h"
#include "oa_switches.h"

#include "lvgl.h"
#include "lvgl_helpers.h"

#define TAG "lvgl_gui"
#define LV_TICK_PERIOD_MS 1

lv_group_t *group_1 = NULL;
lv_group_t *group_2 = NULL;
lv_group_t *group_3 = NULL;
lv_group_t *group_4 = NULL;
lv_group_t *group_4_1 = NULL;
lv_group_t *group_4_2 = NULL;

lv_label_t *label_alias_group_1 = NULL;
lv_label_t *label_code_group_1 = NULL;
lv_label_t *label_time_group_2 = NULL;
lv_label_t *label_sync_time_group_3 = NULL;
lv_label_t *label_ap_name_group_4_1 = NULL;
lv_label_t *label_ap_pass_group_4_1 = NULL;
// add image widget
lv_label_t *label_ip_addr_group_4_2 = NULL;

int read_switch_id();
bool encoder_with_switches(lv_indev_drv_t *drv, lv_indev_data_t *data);
// add button event handler as static
void lvgl_gui_task();

#endif