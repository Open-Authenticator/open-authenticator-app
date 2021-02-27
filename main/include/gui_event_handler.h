#ifndef GUI_EVENT_HANDLER_H
#define GUI_EVENT_HANDLER_H

#include "esp_event.h"
#include "wifi_handler_access_point.h"
#include "wifi_handler_station.h"
#include "ntp.h"
#include "config_http_server.h"

ESP_EVENT_DECLARE_BASE(OPEN_AUTHENTICATOR_EVENTS);

enum
{
    START_SYNC_TIME,
    START_ACCESS_POINT,
    STOP_ACCESS_POINT,
    START_CONFIG_SERVER,
    STOP_CONFIG_SERVER
};

esp_err_t start_gui_event_handler();
esp_err_t stop_gui_event_handler();
esp_err_t post_gui_events(int32_t event_id, void *event_data, size_t event_data_size);

#endif