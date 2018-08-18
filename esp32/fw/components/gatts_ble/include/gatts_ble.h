/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef __GATTS_BLE_H__
#define __GATTS_BLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_BLE_GATTS_SECURITY
    #define WRITE_PERM ESP_GATT_PERM_WRITE_ENC_MITM
    #define READ_PERM ESP_GATT_PERM_READ_ENC_MITM
#else
    #define WRITE_PERM ESP_GATT_PERM_WRITE_ENCRYPTED
    #define READ_PERM ESP_GATT_PERM_READ_ENCRYPTED
#endif
#define RW_PERM READ_PERM|WRITE_PERM

enum {
	IDX_SVC,
	IDX_CHAR_A,
	IDX_CHAR_VAL_A,
	IDX_CHAR_B,
	IDX_CHAR_VAL_B,
	IDX_CHAR_C,
	IDX_CHAR_VAL_C,
	IDX_CHAR_D,
	IDX_CHAR_VAL_D,
	IDX_CHAR_E,
	IDX_CHAR_VAL_E,
	IDX_CHAR_F,
	IDX_CHAR_VAL_F,
	IDX_CHAR_G,
	IDX_CHAR_VAL_G,
	IDX_CHAR_H,
	IDX_CHAR_VAL_H,
	IDX_CHAR_I,
	IDX_CHAR_VAL_I,
	IDX_CHAR_J,
	IDX_CHAR_VAL_J,
	IDX_NB,
};

#define LOG_TAG "DTC_GATT"
#define GATTS_TABLE_TAG "DTC_GATT"

#define PROFILE_NUM                 1
#define PROFILE_IDX					0
#define ESP_APP_ID                  0x0
#define SAMPLE_DEVICE_NAME          "dtc"
#define SVC_INST_ID                 0

//	max length of data in gatt client write or prepare write
#define GATTS_CHAR_VAL_LEN_MAX		500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

#define DECLARE_CHAR( MIDX, UUID, PERM) \
[ IDX_CHAR_ ## MIDX ] = \
{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, \
 (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,\
  CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE,\
 (uint8_t *)&char_prop_read_write_notify}}, \
[ IDX_CHAR_VAL_ ## MIDX ] = \
{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,  (uint8_t *) UUID , PERM,\
  GATTS_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

//event bits
#define TASK_1_BIT        ( 1 << 0 )    //1                                     
#define ALL_SYNC_BITS TASK_1_BIT

typedef struct {
	uint8_t *prepare_buf;
	int prepare_len;
} prepare_type_env_t;

prepare_type_env_t prepare_write_env;

static const uint16_t SERVICE_UUID = 0x00FF;
static const uint16_t CHAR_UUID_BUZZ_STAT = 0xFF01;
static const uint16_t CHAR_UUID_TRIG_MODE = 0xFF02;
static const uint16_t CHAR_UUID_F_INTVL = 0xFF03;
static const uint16_t CHAR_UUID_R_INTVL = 0xFF04;
static const uint16_t CHAR_UUID_RSSI = 0xFF05;
static const uint16_t CHAR_UUID_RSSI_MIN = 0xFF06;
static const uint16_t CHAR_UUID_SOLAR = 0xFF08;
static const uint16_t CHAR_UUID_SOLAR_MIN = 0xFF09;
static const uint16_t CHAR_UUID_PASS = 0xFF07;

static const uint16_t service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid =
    ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write_notify =
    ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ |
    ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t m_ccc[2] = { 0x00, 0x00 };
static const uint8_t char_value[4] = { 0x11, 0x22, 0x33, 0x44 };


void check_char_thresholds_task();

void gatts_profile_event_handler(esp_gatts_cb_event_t event,
				 esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t * param);

void gap_event_handler(esp_gap_ble_cb_event_t event, 
											esp_ble_gap_cb_param_t * param);
void example_prepare_write_event_env(esp_gatt_if_t gatts_if,
	prepare_type_env_t * prepare_write_env, esp_ble_gatts_cb_param_t * param);
void example_exec_write_event_env(prepare_type_env_t * prepare_write_env,
				  							esp_ble_gatts_cb_param_t * param);
void gatts_profile_event_handler(esp_gatts_cb_event_t event, 
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t * param);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
			 								esp_ble_gatts_cb_param_t * param);
void gatts_ble_init();

#endif
