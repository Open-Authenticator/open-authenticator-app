#include "spiffs_handler.h"

static const char *TAG = "spiffs_handler";

esp_err_t init_spiffs()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = OA_MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false};
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

esp_err_t remove_wifi_ap_from_spiffs(char *ssid)
{
    if (ssid == NULL)
    {
        return ESP_FAIL;
    }

    char filepath[30] = WIFI_CRED_PATH;
    FILE *fd = fopen(filepath, "rw+");
    if (fd == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);

        return ESP_FAIL;
    }

    char *wifi_ap_json = calloc(MAX_WIFI_CRED_FILE_SIZE, sizeof(char));

    size_t read_bytes = fread(wifi_ap_json, sizeof(char), MAX_WIFI_CRED_FILE_SIZE, fd);
    fclose(fd);

    if (read_bytes == 0)
    {
        ESP_LOGE(TAG, "Failed to read file : %s", filepath);

        free(wifi_ap_json);
        return ESP_FAIL;
    }
    else if (read_bytes > 0 && verify_wifi_json(wifi_ap_json))
    {
        cJSON *root = cJSON_Parse(wifi_ap_json);
        if (root == NULL)
        {
            free(wifi_ap_json);
            return ESP_FAIL;
        }

        if (cJSON_HasObjectItem(root, "c") && cJSON_GetObjectItem(root, "c")->valueint <= 0)
        {
            cJSON_Delete(root);
            free(wifi_ap_json);
            return ESP_FAIL;
        }

        cJSON *ssid_array = NULL, *pass_array = NULL;
        if (cJSON_HasObjectItem(root, "s") && cJSON_HasObjectItem(root, "p"))
        {
            ssid_array = cJSON_GetObjectItem(root, "s");
            pass_array = cJSON_GetObjectItem(root, "p");
        }
        else
        {
            cJSON_Delete(root);
            free(wifi_ap_json);
            return ESP_FAIL;
        }

        int index = -1;
        for (int i = 0; i < cJSON_GetArraySize(ssid_array); i++)
        {
            if (!strcmp(cJSON_GetArrayItem(ssid_array, i)->valuestring, ssid))
            {
                index = i;
                break;
            }
        }
        if (index != -1)
        {
            cJSON_DeleteItemFromArray(ssid_array, index);
            cJSON_DeleteItemFromArray(pass_array, index);
            cJSON_SetIntValue(cJSON_GetObjectItem(root, "c"), --cJSON_GetObjectItem(root, "c")->valueint);

            char *output = cJSON_PrintUnformatted(root);

            fd = fopen(filepath, "w+");
            if (fd == NULL)
            {
                ESP_LOGE(TAG, "Failed to open file : %s", filepath);

                return ESP_FAIL;
            }
            fwrite(output, sizeof(char), strlen(output), fd);

            cJSON_Delete(root);
            free(wifi_ap_json);
            fclose(fd);
            free(output);
            return ESP_OK;
        }
        else
        {
            cJSON_Delete(root);
            free(wifi_ap_json);
            return ESP_FAIL;
        }
    }

    free(wifi_ap_json);
    return ESP_FAIL;
}

/**
 * returns json with list of access points
 * {"s":["Wifi-AP", "JioFi", "TP-Link"]}
 * 
 */
char *read_wifi_ap_from_spiffs()
{
    char filepath[30] = WIFI_CRED_PATH;
    FILE *fd = fopen(filepath, "r");
    if (fd == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);

        return NULL;
    }

    char *wifi_ap_json = calloc(MAX_WIFI_CRED_FILE_SIZE, sizeof(char));
    char *ap_list = NULL;

    size_t read_bytes = fread(wifi_ap_json, sizeof(char), MAX_WIFI_CRED_FILE_SIZE, fd);
    if (read_bytes == 0)
    {
        ESP_LOGE(TAG, "Failed to read file : %s", filepath);

        free(wifi_ap_json);
        fclose(fd);
        return NULL;
    }
    else if (read_bytes > 0 && verify_wifi_json(wifi_ap_json))
    {
        cJSON *root = cJSON_Parse(wifi_ap_json);
        if (root == NULL)
        {
            free(wifi_ap_json);
            fclose(fd);
            return NULL;
        }

        cJSON_DeleteItemFromObject(root, "c");
        cJSON_DeleteItemFromObject(root, "p");

        ap_list = cJSON_PrintUnformatted(root);

        cJSON_Delete(root);
    }

    free(wifi_ap_json);
    fclose(fd);

    return ap_list;
}

esp_err_t write_wifi_ap_pass_to_spiffs(char *ssid, char *passkey)
{
    if (ssid == NULL || passkey == NULL)
    {
        return ESP_FAIL;
    }

    char filepath[30] = WIFI_CRED_PATH;
    FILE *fd = fopen(filepath, "r");
    if (fd == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);

        return ESP_FAIL;
    }

    char *wifi_ap_json = calloc(MAX_WIFI_CRED_FILE_SIZE, sizeof(char));

    size_t read_bytes = fread(wifi_ap_json, sizeof(char), MAX_WIFI_CRED_FILE_SIZE, fd);
    fclose(fd);

    if (read_bytes == 0)
    {
        ESP_LOGE(TAG, "Failed to read file : %s", filepath);

        free(wifi_ap_json);
        return ESP_FAIL;
    }
    else if (read_bytes > 0 && verify_wifi_json(wifi_ap_json))
    {
        cJSON *root = cJSON_Parse(wifi_ap_json);
        if (root == NULL)
        {
            free(wifi_ap_json);
            return ESP_FAIL;
        }

        cJSON *ssid_array = NULL, *pass_array = NULL;
        if (cJSON_HasObjectItem(root, "s") && cJSON_HasObjectItem(root, "p"))
        {
            ssid_array = cJSON_GetObjectItem(root, "s");
            pass_array = cJSON_GetObjectItem(root, "p");
        }
        else
        {
            cJSON_Delete(root);
            free(wifi_ap_json);
            return ESP_FAIL;
        }

        int index = -1;
        for (int i = 0; i < cJSON_GetArraySize(ssid_array); i++)
        {
            if (!strcmp(cJSON_GetArrayItem(ssid_array, i)->valuestring, ssid))
            {
                index = i;
                break;
            }
        }
        if (index == -1)
        {
            cJSON_AddItemToArray(ssid_array, cJSON_CreateString(ssid));
            cJSON_AddItemToArray(pass_array, cJSON_CreateString(passkey));
            cJSON_SetIntValue(cJSON_GetObjectItem(root, "c"), ++cJSON_GetObjectItem(root, "c")->valueint);

            char *output = cJSON_PrintUnformatted(root);

            fd = fopen(filepath, "w+");
            if (fd == NULL)
            {
                ESP_LOGE(TAG, "Failed to open file : %s", filepath);

                return ESP_FAIL;
            }
            fwrite(output, sizeof(char), strlen(output), fd);

            cJSON_Delete(root);
            free(wifi_ap_json);
            fclose(fd);
            free(output);
            return ESP_OK;
        }
        else
        {
            cJSON_Delete(root);
            free(wifi_ap_json);
            return ESP_FAIL;
        }
    }

    free(wifi_ap_json);
    return ESP_FAIL;
}

esp_err_t remove_totp_alias_from_spiffs(char *alias)
{
    if (alias == NULL)
    {
        return ESP_FAIL;
    }

    char filepath[30] = TOTP_KEY_PATH;
    FILE *fd = fopen(filepath, "rw+");
    if (fd == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);

        return ESP_FAIL;
    }

    char *totp_alias_json = calloc(MAX_TOTP_CRED_FILE_SIZE, sizeof(char));

    size_t read_bytes = fread(totp_alias_json, sizeof(char), MAX_TOTP_CRED_FILE_SIZE, fd);
    fclose(fd);

    if (read_bytes == 0)
    {
        ESP_LOGE(TAG, "Failed to read file : %s", filepath);

        free(totp_alias_json);
        return ESP_FAIL;
    }
    else if (read_bytes > 0 && verify_totp_json(totp_alias_json))
    {
        cJSON *root = cJSON_Parse(totp_alias_json);
        if (root == NULL)
        {
            free(totp_alias_json);
            return ESP_FAIL;
        }

        if (cJSON_HasObjectItem(root, "c") && cJSON_GetObjectItem(root, "c")->valueint <= 0)
        {
            cJSON_Delete(root);
            free(totp_alias_json);
            return ESP_FAIL;
        }

        cJSON *alias_array = NULL, *key_array = NULL;
        if (cJSON_HasObjectItem(root, "a") && cJSON_HasObjectItem(root, "k"))
        {
            alias_array = cJSON_GetObjectItem(root, "a");
            key_array = cJSON_GetObjectItem(root, "k");
        }
        else
        {
            cJSON_Delete(root);
            free(totp_alias_json);
            return ESP_FAIL;
        }

        int index = -1;
        for (int i = 0; i < cJSON_GetArraySize(alias_array); i++)
        {
            if (!strcmp(cJSON_GetArrayItem(alias_array, i)->valuestring, alias))
            {
                index = i;
                break;
            }
        }
        if (index != -1)
        {
            cJSON_DeleteItemFromArray(alias_array, index);
            cJSON_DeleteItemFromArray(key_array, index);
            cJSON_SetIntValue(cJSON_GetObjectItem(root, "c"), --cJSON_GetObjectItem(root, "c")->valueint);

            char *output = cJSON_PrintUnformatted(root);

            fd = fopen(filepath, "w+");
            if (fd == NULL)
            {
                ESP_LOGE(TAG, "Failed to open file : %s", filepath);

                return ESP_FAIL;
            }
            fwrite(output, sizeof(char), strlen(output), fd);

            cJSON_Delete(root);
            free(totp_alias_json);
            fclose(fd);
            free(output);
            return ESP_OK;
        }
        else
        {
            cJSON_Delete(root);
            free(totp_alias_json);
            return ESP_FAIL;
        }
    }

    free(totp_alias_json);
    return ESP_FAIL;
}

char *read_totp_alias_from_spiffs()
{
    char filepath[30] = TOTP_KEY_PATH;
    FILE *fd = fopen(filepath, "r");
    if (fd == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);

        return NULL;
    }

    char *totp_alias_json = calloc(MAX_TOTP_CRED_FILE_SIZE, sizeof(char));
    char *alias_list = NULL;

    size_t read_bytes = fread(totp_alias_json, sizeof(char), MAX_TOTP_CRED_FILE_SIZE, fd);
    if (read_bytes == 0)
    {
        ESP_LOGE(TAG, "Failed to read file : %s", filepath);

        free(totp_alias_json);
        fclose(fd);
        return NULL;
    }
    else if (read_bytes > 0 && verify_totp_json(totp_alias_json))
    {
        cJSON *root = cJSON_Parse(totp_alias_json);
        if (root == NULL)
        {
            free(totp_alias_json);
            fclose(fd);
            return NULL;
        }

        cJSON_DeleteItemFromObject(root, "c");
        cJSON_DeleteItemFromObject(root, "k");

        alias_list = cJSON_PrintUnformatted(root);

        cJSON_Delete(root);
    }

    free(totp_alias_json);
    fclose(fd);

    return alias_list;
}

esp_err_t write_totp_alias_key_to_spiffs(char *alias, char *key)
{
    if (alias == NULL || key == NULL)
    {
        return ESP_FAIL;
    }
    
    char filepath[30] = TOTP_KEY_PATH;
    FILE *fd = fopen(filepath, "r");
    if (fd == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);

        return ESP_FAIL;
    }

    char *totp_alias_json = calloc(MAX_TOTP_CRED_FILE_SIZE, sizeof(char));

    size_t read_bytes = fread(totp_alias_json, sizeof(char), MAX_TOTP_CRED_FILE_SIZE, fd);
    fclose(fd);

    if (read_bytes == 0)
    {
        ESP_LOGE(TAG, "Failed to read file : %s", filepath);

        free(totp_alias_json);
        return ESP_FAIL;
    }
    else if (read_bytes > 0 && verify_totp_json(totp_alias_json))
    {
        cJSON *root = cJSON_Parse(totp_alias_json);
        if (root == NULL)
        {
            free(totp_alias_json);
            return ESP_FAIL;
        }

        cJSON *alias_array = NULL, *key_array = NULL;
        if (cJSON_HasObjectItem(root, "a") && cJSON_HasObjectItem(root, "k"))
        {
            alias_array = cJSON_GetObjectItem(root, "a");
            key_array = cJSON_GetObjectItem(root, "k");
        }
        else
        {
            cJSON_Delete(root);
            free(totp_alias_json);
            return ESP_FAIL;
        }

        int index = -1;
        for (int i = 0; i < cJSON_GetArraySize(alias_array); i++)
        {
            if (!strcmp(cJSON_GetArrayItem(alias_array, i)->valuestring, alias))
            {
                index = i;
                break;
            }
        }
        if (index == -1)
        {
            cJSON_AddItemToArray(alias_array, cJSON_CreateString(alias));
            cJSON_AddItemToArray(key_array, cJSON_CreateString(key));
            cJSON_SetIntValue(cJSON_GetObjectItem(root, "c"), ++cJSON_GetObjectItem(root, "c")->valueint);

            char *output = cJSON_PrintUnformatted(root);

            fd = fopen(filepath, "w+");
            if (fd == NULL)
            {
                ESP_LOGE(TAG, "Failed to open file : %s", filepath);

                return ESP_FAIL;
            }
            fwrite(output, sizeof(char), strlen(output), fd);

            cJSON_Delete(root);
            free(totp_alias_json);
            fclose(fd);
            free(output);
            return ESP_OK;
        }
        else
        {
            cJSON_Delete(root);
            free(totp_alias_json);
            return ESP_FAIL;
        }
    }

    free(totp_alias_json);
    return ESP_FAIL;
}

char *read_wifi_creds()
{
    char filepath[30] = WIFI_CRED_PATH;
    FILE *fd = fopen(filepath, "r");
    if (fd == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);

        return NULL;
    }

    char *wifi_ap_json = calloc(MAX_WIFI_CRED_FILE_SIZE, sizeof(char));

    size_t read_bytes = fread(wifi_ap_json, sizeof(char), MAX_WIFI_CRED_FILE_SIZE, fd);
    if (read_bytes == 0)
    {
        ESP_LOGE(TAG, "Failed to read file : %s", filepath);

        free(wifi_ap_json);
        fclose(fd);
        return NULL;
    }
    else if (read_bytes > 0)
    {
        fclose(fd);
        return wifi_ap_json;
    }

    free(wifi_ap_json);
    fclose(fd);
    return NULL;
}

totp_key_creds totp_key(int key_id);

bool verify_wifi_json(char *input_json) { return true; }
bool verify_totp_json(char *input_json) { return true; }
