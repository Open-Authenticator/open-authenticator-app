#ifndef SPIFFS_HANDLER_H
#define SPIFFS_HANDLER_H

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#define OA_MOUNT_POINT "/oa_store"
#define WIFI_CRED_PATH OA_MOUNT_POINT "/wifi"
#define TOTP_KEY_PATH OA_MOUNT_POINT "/totp_key"
#define WEBSITE_PATH OA_MOUNT_POINT "/www"
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)

typedef struct key_creds 
{
    char* alias;
    char* key;
}totp_key_creds;

esp_err_t init_spiffs();
char* wifi_cred_json();
totp_key_creds totp_key(int key_id);

#endif