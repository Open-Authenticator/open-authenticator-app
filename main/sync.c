#include "sync.h"

void sync_time()
{
    rtc_ext_init();
        
    while (1)
    {
        time_t time = rtc_ext_get_time();
        struct timeval tm = {.tv_sec = time};
        settimeofday(&tm, NULL);
        ESP_LOGI("rtc", "%s: %lld", "periodic time update from RTC", (long long)time);

        ntp_get_time();
        
        vTaskDelay(3600000 / portTICK_PERIOD_MS);
    }
}