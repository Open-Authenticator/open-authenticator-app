#include "lvgl_gui.h"

int read_switch_id()

bool encoder_with_switches(lv_indev_drv_t *drv, lv_indev_data_t *data)

static void switch_event_handler_cb(lv_obj_t *obj, lv_event_t event)

void lvgl_gui_task()

static void lv_tick_task(void *arg)
{
    lv_task_handler();
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
