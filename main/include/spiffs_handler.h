#ifndef SPIFFS_HANDLER_H
#define SPIFFS_HANDLER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "cJSON.h"

#define OA_MOUNT_POINT "/oa_store"
#define WIFI_CRED_PATH OA_MOUNT_POINT "/wifi.json"
#define TOTP_KEY_PATH OA_MOUNT_POINT "/totp_key.json"
#define WEBSITE_PATH OA_MOUNT_POINT "/www"
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define MAX_WIFI_CRED_FILE_SIZE 1040
#define MAX_TOTP_CRED_FILE_SIZE 1040
typedef struct key_creds 
{
    char alias[20];
    char key[64];
}totp_key_creds;

esp_err_t init_spiffs();
esp_err_t remove_wifi_ap_from_spiffs(char *ssid);
char* read_wifi_ap_from_spiffs();
esp_err_t write_wifi_ap_pass_to_spiffs(char *ssid, char *passkey);
esp_err_t remove_totp_alias_from_spiffs(char *alias);
char* read_totp_alias_from_spiffs();
esp_err_t write_totp_alias_key_to_spiffs(char *alias, char *key);
char* read_wifi_creds();
esp_err_t read_totp_key(int key_id, totp_key_creds *key_creds);
int read_totp_key_count();
bool verify_wifi_json(char* input_json);
bool verify_totp_json(char* input_json);

#endif