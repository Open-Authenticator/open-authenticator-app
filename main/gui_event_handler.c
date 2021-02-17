#include "gui_event_handler.h"
#include "esp_event_base.h"

ESP_EVENT_DEFINE_BASE(OPEN_AUTHENTICATOR_EVENTS);

static esp_event_loop_handle_t gui_event_handle = NULL;
static const char* TAG = "gui_event_handler";

esp_err_t start_gui_event_handler();
esp_err_t stop_gui_event_handler();
esp_err_t post_gui_events(int32_t event_id, void *event_data, size_t event_data_size);
