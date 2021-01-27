#ifndef __LVGL_GUI_H_
#define __LVGL_GUI_H_

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

#define LV_TICK_PERIOD_MS 1

int read_switch_id();
bool encoder_with_switches(lv_indev_drv_t *drv, lv_indev_data_t *data);
void lvgl_gui_task();

#endif