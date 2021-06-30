#include "config_http_server.h"

static const char *TAG = "config_http_server";
static char scratch[SCRATCH_BUFSIZE];
static httpd_handle_t server = NULL;

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html"))
    {
        type = "text/html";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".js"))
    {
        type = "application/javascript";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".css"))
    {
        type = "text/css";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".png"))
    {
        type = "image/png";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".ico"))
    {
        type = "image/x-icon";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".svg"))
    {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX] = WEBSITE_PATH;

    if (strlen(req->uri) > 0 && req->uri[strlen(req->uri) - 1] == '/')
    {
        strlcat(filepath, "/index.html", sizeof(filepath));
    }
    else
    {
        strlcat(filepath, req->uri, sizeof(filepath));
    }

    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1)
    {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = scratch;
    memset(scratch, '\0', SCRATCH_BUFSIZE);
    ssize_t read_bytes;
    do
    {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1)
        {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        }
        else if (read_bytes > 0)
        {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK)
            {
                close(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t remove_ap_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = scratch;
    memset(scratch, '\0', SCRATCH_BUFSIZE);
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root != NULL && cJSON_HasObjectItem(root, "s") && cJSON_IsString(cJSON_GetObjectItem(root, "s")))
    {
        if (remove_wifi_ap_from_spiffs(cJSON_GetObjectItem(root, "s")->valuestring) == ESP_OK)
        {
            httpd_resp_sendstr(req, "removed access point from list");
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "unable to remove access point from list");
        }
    }
    else
    {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "unable to remove access point from list");
    }

    return ESP_OK;
}

static esp_err_t read_ap_list_handler(httpd_req_t *req)
{
    char *data = read_wifi_ap_from_spiffs();
    httpd_resp_set_type(req, "application/json");
    if (data != NULL)
    {
        httpd_resp_sendstr(req, data);
    }
    else
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read access point values");
    }

    free(data);

    return ESP_OK;
}

static esp_err_t write_ap_pass_list_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = scratch;
    memset(scratch, '\0', SCRATCH_BUFSIZE);
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root != NULL && cJSON_HasObjectItem(root, "s") && cJSON_IsString(cJSON_GetObjectItem(root, "s")) &&
        cJSON_HasObjectItem(root, "p") && cJSON_IsString(cJSON_GetObjectItem(root, "p")))
    {
        if (write_wifi_ap_pass_to_spiffs(cJSON_GetObjectItem(root, "s")->valuestring, cJSON_GetObjectItem(root, "p")->valuestring) == ESP_OK)
        {
            httpd_resp_sendstr(req, "added access point to list");
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "unable to add access point to list");
        }
    }
    else
    {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "unable to add access point to list");
    }

    return ESP_OK;
}

static esp_err_t remove_alias_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = scratch;
    memset(scratch, '\0', SCRATCH_BUFSIZE);
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root != NULL && cJSON_HasObjectItem(root, "a") && cJSON_IsString(cJSON_GetObjectItem(root, "a")))
    {
        if (remove_totp_alias_from_spiffs(cJSON_GetObjectItem(root, "a")->valuestring) == ESP_OK)
        {
            httpd_resp_sendstr(req, "removed alias-key from list");
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "unable to remove alias-key from list");
        }
    }
    else
    {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "unable to remove alias-key from list");
    }

    return ESP_OK;
}

static esp_err_t read_alias_list_handler(httpd_req_t *req)
{
    char *data = read_totp_alias_from_spiffs();
    httpd_resp_set_type(req, "application/json");
    if (data != NULL)
    {
        httpd_resp_sendstr(req, data);
    }
    else
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read totp alias values");
    }

    free(data);

    return ESP_OK;
}

static esp_err_t ping_device_status(httpd_req_t *req)
{
    httpd_resp_sendstr(req, "working");

    return ESP_OK;
}

static esp_err_t write_alias_key_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = scratch;
    memset(scratch, '\0', SCRATCH_BUFSIZE);
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root != NULL && cJSON_HasObjectItem(root, "a") && cJSON_IsString(cJSON_GetObjectItem(root, "a")) &&
        cJSON_HasObjectItem(root, "k") && cJSON_IsString(cJSON_GetObjectItem(root, "k")))
    {
        if (write_totp_alias_key_to_spiffs(cJSON_GetObjectItem(root, "a")->valuestring, cJSON_GetObjectItem(root, "k")->valuestring) == ESP_OK)
        {
            httpd_resp_sendstr(req, "added alias-key to list");
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "unable to add alias-key to list");
        }
    }
    else
    {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "unable to add alias-key to list");
    }

    return ESP_OK;
}

static esp_err_t start_config_http_server_private()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "start server failed");
        return ESP_FAIL;
    }

    // AP endpoints
    httpd_uri_t remove_ap_post_uri = {
        .uri = "/api/v1/ap/remove",
        .method = HTTP_POST,
        .handler = remove_ap_handler,
        .user_ctx = NULL};
    if (httpd_register_uri_handler(server, &remove_ap_post_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register post uri failed");
        return ESP_FAIL;
    }

    httpd_uri_t read_ap_list_get_uri = {
        .uri = "/api/v1/ap/read",
        .method = HTTP_GET,
        .handler = read_ap_list_handler,
        .user_ctx = NULL};
    if (httpd_register_uri_handler(server, &read_ap_list_get_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register post uri failed");
        return ESP_FAIL;
    }

    httpd_uri_t write_ap_pass_list_post_uri = {
        .uri = "/api/v1/ap/write",
        .method = HTTP_POST,
        .handler = write_ap_pass_list_handler,
        .user_ctx = NULL};
    if (httpd_register_uri_handler(server, &write_ap_pass_list_post_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register post uri failed");
        return ESP_FAIL;
    }

    // key endpoints
    httpd_uri_t remove_alias_post_uri = {
        .uri = "/api/v1/key/remove",
        .method = HTTP_POST,
        .handler = remove_alias_handler,
        .user_ctx = NULL};
    if (httpd_register_uri_handler(server, &remove_alias_post_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register post uri failed");
        return ESP_FAIL;
    }

    httpd_uri_t read_alias_list_get_uri = {
        .uri = "/api/v1/key/read",
        .method = HTTP_GET,
        .handler = read_alias_list_handler,
        .user_ctx = NULL};
    if (httpd_register_uri_handler(server, &read_alias_list_get_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register post uri failed");
        return ESP_FAIL;
    }

    httpd_uri_t write_alias_key_list_post_uri = {
        .uri = "/api/v1/key/write",
        .method = HTTP_POST,
        .handler = write_alias_key_handler,
        .user_ctx = NULL};
    if (httpd_register_uri_handler(server, &write_alias_key_list_post_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register post uri failed");
        return ESP_FAIL;
    }

    httpd_uri_t ping_device_status_get_uri = {
        .uri = "/api/v1/device/status",
        .method = HTTP_GET,
        .handler = ping_device_status,
        .user_ctx = NULL};
    if (httpd_register_uri_handler(server, &ping_device_status_get_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register post uri failed");
        return ESP_FAIL;
    }

    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = NULL};
    if (httpd_register_uri_handler(server, &common_get_uri) != ESP_OK)
    {
        ESP_LOGE(TAG, "register get uri failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void start_config_http_server()
{
    ESP_ERROR_CHECK(start_config_http_server_private());
}

void stop_config_http_server()
{
    if (server != NULL)
    {
        httpd_stop(server);
        server = NULL;
    }

    ESP_LOGI(TAG, "stopped httpd server");
}