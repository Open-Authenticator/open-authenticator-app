#ifndef TOTP_H
#define TOTP_H

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mbedtls/md.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

size_t decode_b32key(uint8_t **k, size_t len);
void totp_init(mbedtls_md_type_t md_type);
void totp_generate(char* secret_key, uint64_t counter, int N, char* message_hash);
void totp_free();

#endif
