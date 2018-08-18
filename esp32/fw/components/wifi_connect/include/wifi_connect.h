/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef __WIFI_CONNECT_H__
#define __WIFI_CONNECT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lwip/sockets.h>
#include <errno.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#define DTC_ESP_WIFI_MODE_AP   "TRUE"   //TRUE:AP FALSE:STA
#define DTC_ESP_WIFI_SSID      "DTCAP"
#define DTC_ESP_WIFI_PASS      "12345678" //overwritten later
#define DTC_MAX_STA_CONN       1
#define PORT_NUMBER 5000

void wifi_connect_init(uint64_t pass_int);
void wifi_connect_destroy();
void socket_server();
#endif
