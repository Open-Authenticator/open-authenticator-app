#include "gui_event_handler.h"
#include "esp_event_base.h"

ESP_EVENT_DEFINE_BASE(OPEN_AUTHENTICATOR_EVENTS);

static esp_event_loop_handle_t gui_event_handle = NULL;
static const char *TAG = "gui_event_handler";

static bool access_point_mode_enabled = false;

static void action_connect_to_wifi()
{
    char *wifi_creds = read_wifi_creds();
    if (wifi_creds == NULL)
    {
        return;
    }     

    esp_err_t err;
    do
    {
        err = start_wifi_station(wifi_creds);
        vTaskDelay(pdMS_TO_TICKS(1000));
    } while (err == WIFI_ERR_ALREADY_RUNNING);
    
    if (err == ESP_OK)
    {
        ntp_get_time();
    }
    stop_wifi_station();
    free(wifi_creds);
}

static void gui_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    if (id == START_SYNC_TIME)
    {
        if (*(bool *)event_data)
        { 
            xEventGroupSetBits(gui_event_group, S_SYNC_TIME_BIT);
        }
        if (!access_point_mode_enabled)
        {
            action_connect_to_wifi();
        }
        if (*(bool *)event_data)
        {
            xEventGroupSetBits(gui_event_group, E_SYNC_TIME_BIT);
        }
    }
    else if (id == START_ACCESS_POINT)
    {
        xEventGroupSetBits(gui_event_group, S_START_ACCESS_POINT);
        if (start_wifi_access_point("open-authenticator", (char *)event_data) == ESP_OK)
        {
            xEventGroupSetBits(gui_event_group, E_START_ACCESS_POINT);
            access_point_mode_enabled = true;
        }
        else
        {
            xEventGroupSetBits(gui_event_group, K_START_ACCESS_POINT);
            access_point_mode_enabled = false;
        }
    }
    else if (id == STOP_ACCESS_POINT)
    {
        xEventGroupSetBits(gui_event_group, S_STOP_ACCESS_POINT);
        stop_wifi_access_point();
        xEventGroupSetBits(gui_event_group, E_STOP_ACCESS_POINT);
        access_point_mode_enabled = false;
    }
    else if (id == START_CONFIG_SERVER)
    {
        xEventGroupSetBits(gui_event_group, S_START_CONFIG_SERVER);
        start_config_http_server();
        xEventGroupSetBits(gui_event_group, E_START_CONFIG_SERVER);
    }
    else if (id == STOP_CONFIG_SERVER)
    {
        xEventGroupSetBits(gui_event_group, S_STOP_CONFIG_SERVER);
        stop_config_http_server();
        xEventGroupSetBits(gui_event_group, E_STOP_CONFIG_SERVER);
    }
}

esp_err_t start_gui_event_handler()
{
    gui_event_group = xEventGroupCreate();

    esp_event_loop_args_t gui_event_handle_args = {
        .queue_size = 10,
        .task_name = "gui_event_handler",
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY};
    ESP_ERROR_CHECK(esp_event_loop_create(&gui_event_handle_args, &gui_event_handle));

    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(gui_event_handle, OPEN_AUTHENTICATOR_EVENTS, ESP_EVENT_ANY_ID, &gui_event_handler, NULL, NULL));

    return ESP_OK;
}

esp_err_t stop_gui_event_handler();

esp_err_t post_gui_events(int32_t event_id, void *event_data, size_t event_data_size)
{
    return esp_event_post_to(gui_event_handle, OPEN_AUTHENTICATOR_EVENTS, event_id, event_data, event_data_size, portMAX_DELAY);
}
