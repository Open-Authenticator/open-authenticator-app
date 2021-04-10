#ifndef GUI_EVENT_HANDLER_H
#define GUI_EVENT_HANDLER_H

#include "esp_event.h"
#include "freertos/event_groups.h"
#include "wifi_handler_access_point.h"
#include "wifi_handler_station.h"
#include "ntp.h"
#include "config_http_server.h"
#include "spiffs_handler.h"

#define S_SYNC_TIME_BIT BIT0
#define E_SYNC_TIME_BIT BIT1

#define S_START_ACCESS_POINT BIT2
#define E_START_ACCESS_POINT BIT3
#define K_START_ACCESS_POINT BIT4

#define S_STOP_ACCESS_POINT BIT5
#define E_STOP_ACCESS_POINT BIT6

#define S_START_CONFIG_SERVER BIT7
#define E_START_CONFIG_SERVER BIT8

#define S_STOP_CONFIG_SERVER BIT9
#define E_STOP_CONFIG_SERVER BIT10

EventGroupHandle_t gui_event_group;

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