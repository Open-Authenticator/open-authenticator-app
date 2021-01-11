#include "rtc.h"

static i2c_dev_t dev;

void rtc_ext_init(int sda, int scl)
{
    ESP_ERROR_CHECK(i2cdev_init());

    memset(&dev, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(ds3231_init_desc(&dev, 1, sda, scl));
}

void rtc_ext_set_time(time_t time)
{
    while(ds3231_set_time(&dev, localtime(&time)) != ESP_OK)
    {
        ESP_LOGE("rtc", "%s: %s", "setting rtc time", asctime(localtime(&time)));
    }

    ESP_LOGI("rtc", "%lld", (long long)time);
}

time_t rtc_ext_get_time()
{
    struct tm time;
    while(ds3231_get_time(&dev, &time) != ESP_OK);

    return mktime(&time);
}
