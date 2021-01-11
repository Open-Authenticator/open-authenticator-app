#ifndef RTC_H
#define RTC_H

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "ds3231.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define SDA_GPIO 5
#define SCL_GPIO 18

void rtc_ext_init();
void rtc_ext_set_time(time_t time);
time_t rtc_ext_get_time();

#endif