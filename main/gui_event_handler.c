#include "gui_event_handler.h"
#include "esp_event_base.h"

ESP_EVENT_DEFINE_BASE(OPEN_AUTHENTICATOR_EVENTS);

static esp_event_loop_handle_t gui_event_handle = NULL;
static const char *TAG = "gui_event_handler";

static void action_connect_to_wifi()
{    
    while(start_wifi_station("{\"c\":1,\"s\":[\"P\"],\"p\":[\"v\"]}") == WIFI_ERR_ALREADY_RUNNING)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ntp_get_time();
    stop_wifi_station();
}

static void gui_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    // int iteration = *((int*) event_data);

    if (id == START_SYNC_TIME)
    {
        ESP_LOGI(TAG, "wifi event started succesfully");
        action_connect_to_wifi();
        ESP_LOGI(TAG, "wifi event finished succesfully");
    }
    // else if (id == START_ACCESS_POINT)
    // else if (id == STOP_ACCESS_POINT)
    // else if (id == START_CONFIG_SERVER)
    // else if (id == STOP_CONFIG_SERVER)
}

esp_err_t start_gui_event_handler()
{
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

esp_err_t post_gui_events(int32_t event_id, void *event_data)
{
    return esp_event_post_to(gui_event_handle, OPEN_AUTHENTICATOR_EVENTS, event_id, event_data, sizeof(event_data), portMAX_DELAY);
}
