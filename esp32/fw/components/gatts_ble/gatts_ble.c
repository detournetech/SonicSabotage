#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "gatts_ble.h"
#include "gpio_handler.h"
#include "esp_gatt_common_api.h"

#include "wifi_connect.h"

static TaskHandle_t check_chars_thandle;
static bool connected = false;
uint8_t adv_config_done = 0;
int8_t last_rssi_val = 0;
uint16_t m_handles[IDX_NB];

typedef enum mode_internal {
	MODE_IDLE,		//can receive manual beep events
	MODE_FWUPDATE,	//fw update triggered, no return to idle/loop until reset
	MODE_LOOP		//characteristic notify & threshold check loop is running
} mode_internal_t;

typedef enum mode_loop_type {
	LOOP_FIXEDINT,
	LOOP_RANDINT,
	LOOP_SOLAR,
	LOOP_RSSI,
	LOOP_NONE
} mode_loop_type_t;

static mode_internal_t mode_internal = MODE_IDLE;
static mode_loop_type_t mode_loop = LOOP_NONE;

static uint8_t threshold_val = 0;
static int rand_array[3600];
static uint64_t int_pass;

#define CONFIG_SET_RAW_ADV_DATA
#ifdef CONFIG_SET_RAW_ADV_DATA
uint8_t raw_adv_data[] = {
	// flags 
	0x02, 0x01, 0x06,
	// tx power
	0x02, 0x0a, 0xeb,
	// service uuid 
	0x03, 0x03, 0xFF, 0x00,
	// device name 
	0x04, 0x09, 'd', 't', 'c'
};

uint8_t raw_scan_rsp_data[] = {
	// flags 
	0x02, 0x01, 0x06,
	// tx power 
	0x02, 0x0a, 0xeb,
	// service uuid 
	0x03, 0x03, 0xFF, 0x00
};

#else
uint8_t service_uuid[16] = {
	// LSB <-----------------------------------------------> MSB 
	//first uuid, 16bit, [12],[13] is the value
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00,
	0xFF, 0x00, 0x00, 0x00,
};

/* The length of adv data must be less than 31 bytes */
esp_ble_adv_data_t adv_data = {
	.set_scan_rsp = false,
	.include_name = true,
	.include_txpower = true,
	.min_interval = 0x20,
	.max_interval = 0x40,
	.appearance = 0x00,
	.manufacturer_len = 0,	//TEST_MANUFACTURER_DATA_LEN,
	.p_manufacturer_data = NULL,	//test_manufacturer,
	.service_data_len = 0,
	.p_service_data = NULL,
	.service_uuid_len = sizeof(service_uuid),
	.p_service_uuid = service_uuid,
	.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
esp_ble_adv_data_t scan_rsp_data = {
	.set_scan_rsp = true,
	.include_name = true,
	.include_txpower = true,
	.min_interval = 0x20,
	.max_interval = 0x40,
	.appearance = 0x00,
	.manufacturer_len = 0,	//TEST_MANUFACTURER_DATA_LEN,
	.p_manufacturer_data = NULL,	//&test_manufacturer[0],
	.service_data_len = 0,
	.p_service_data = NULL,
	.service_uuid_len = 16,
	.p_service_uuid = service_uuid,
	.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
#endif				/* CONFIG_SET_RAW_ADV_DATA */

esp_ble_adv_params_t adv_params = {
	.adv_int_min = 0x20,
	.adv_int_max = 0x40,
	.adv_type = ADV_TYPE_IND,
	.own_addr_type = BLE_ADDR_TYPE_RANDOM, // ?? attempt to make bonds endure
	.channel_map = ADV_CHNL_ALL,
	.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
	esp_gatts_cb_t gatts_cb;
	uint16_t gatts_if;
	uint16_t app_id;
	uint16_t conn_id;
	uint16_t service_handle;
	esp_gatt_srvc_id_t service_id;
	uint16_t char_handle;
	esp_bt_uuid_t char_uuid;
	esp_gatt_perm_t perm;
	esp_gatt_char_prop_t property;
	uint16_t descr_handle;
	esp_bt_uuid_t descr_uuid;
};

esp_ble_conn_update_params_t conn_params;

void gatts_profile_event_handler(esp_gatts_cb_event_t event, 
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t * param);

// this array will store the gatts_if returned by ESP_GATTS_REG_EVT 
struct gatts_profile_inst m_profile_tab[PROFILE_NUM] = {
	[PROFILE_IDX] = {
			     .gatts_cb = gatts_profile_event_handler,
			     .gatts_if = ESP_GATT_IF_NONE,	//initial value 
			     },
};

// database description of service + attributes
const esp_gatts_attr_db_t gatt_db[IDX_NB] = {
	// Service Declaration

	[IDX_SVC] = {{ESP_GATT_AUTO_RSP}, 
		{ESP_UUID_LEN_16, (uint8_t *)& service_uuid, ESP_GATT_PERM_READ, 
		sizeof(uint16_t), sizeof(SERVICE_UUID), (uint8_t *) & SERVICE_UUID}},
	DECLARE_CHAR(A, &CHAR_UUID_BUZZ_STAT,	RW_PERM )
    DECLARE_CHAR(B, &CHAR_UUID_TRIG_MODE,	RW_PERM )
    DECLARE_CHAR(C, &CHAR_UUID_F_INTVL,		RW_PERM )
    DECLARE_CHAR(D, &CHAR_UUID_R_INTVL,		RW_PERM )
    DECLARE_CHAR(E, &CHAR_UUID_RSSI,		READ_PERM )
    DECLARE_CHAR(F, &CHAR_UUID_RSSI_MIN,	RW_PERM )
    DECLARE_CHAR(H, &CHAR_UUID_SOLAR,		READ_PERM )
    DECLARE_CHAR(I, &CHAR_UUID_SOLAR_MIN, 	RW_PERM )
    DECLARE_CHAR(J, &CHAR_UUID_PASS, 		READ_PERM )
};

static void show_bonded_devices(void)
{
	int dev_num = esp_ble_get_bond_device_num();
	esp_ble_bond_dev_t *dev_list =
	    (esp_ble_bond_dev_t *) malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
	esp_ble_get_bond_device_list(&dev_num, dev_list);
	ESP_LOGI(GATTS_TABLE_TAG, "Bonded devices number : %d\n", dev_num);

	ESP_LOGI(GATTS_TABLE_TAG, "Bonded devices list : %d\n", dev_num);
	for (int i = 0; i < dev_num; i++) {
		esp_log_buffer_hex(GATTS_TABLE_TAG, (void *)dev_list[i].bd_addr,
				   sizeof(esp_bd_addr_t));
	}
	free(dev_list);
}

static void __attribute__ ((unused)) remove_all_bonded_devices(void)
{
	int dev_num = esp_ble_get_bond_device_num();

	esp_ble_bond_dev_t *dev_list =
	    (esp_ble_bond_dev_t *) malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
	esp_ble_get_bond_device_list(&dev_num, dev_list);
	for (int i = 0; i < dev_num; i++) {
		esp_ble_remove_bond_device(dev_list[i].bd_addr);
	}

	free(dev_list);
}

void blink_out_code(int passkey)
{
	int y = 100000;
	int x = 1000000;
	int gpios[3] = { GPIO_RED, GPIO_GREEN, GPIO_BLUE };
	for (int i = 1; i <= 6; i++) {
		int j = (passkey % x) / y;
		y /= 10;
		x /= 10;
		ESP_LOGI(GATTS_TABLE_TAG, "key digit:%d\n", j);
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		if (j == 0) {
			gpio_blink_white();
		} else {
			for (int k = 1; k <= j; k++) {
				gpio_blink_one(gpios[i % 3]);
			}
		}
	}
}

void
gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t * param)
{
	switch (event) {
#ifdef CONFIG_SET_RAW_ADV_DATA
	case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
		adv_config_done &= (~ADV_CONFIG_FLAG);
		if (adv_config_done == 0) {
			esp_ble_gap_start_advertising(&adv_params);
		}
		break;
	case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
		adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
		if (adv_config_done == 0) {
			esp_ble_gap_start_advertising(&adv_params);
		}
		break;
#else
	case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
		adv_config_done &= (~ADV_CONFIG_FLAG);
		if (adv_config_done == 0) {
			esp_ble_gap_start_advertising(&adv_params);
		}
		break;
	case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
		adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
		if (adv_config_done == 0) {
			esp_ble_gap_start_advertising(&adv_params);
		}
		break;
#endif
	case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
		// advertising start (success or fail) event
		if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
			ESP_LOGE(GATTS_TABLE_TAG, "advertising start failed");
		} else {
			ESP_LOGI(GATTS_TABLE_TAG, "advertising start successfully");
		}
		break;
	case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
		if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
			ESP_LOGE(GATTS_TABLE_TAG, "Advertising stop failed");
		} else {
			ESP_LOGI(GATTS_TABLE_TAG, "Stop adv successfully\n");
		}
		break;
	case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
		ESP_LOGI(GATTS_TABLE_TAG,
			"stat=%d min_int=%d max_int=%d conn_int=%d latency=%d timeout=%d",
			param->update_conn_params.status, 
			param->update_conn_params.min_int,
			param->update_conn_params.max_int, 
			param->update_conn_params.conn_int,
			param->update_conn_params.latency,
			param->update_conn_params.timeout);
		break;
	case ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT:
		if (param->read_rssi_cmpl.status != ESP_BT_STATUS_SUCCESS) {
			ESP_LOGE(GATTS_TABLE_TAG, "remote rssi read failed");
		} else {
			//ESP_LOGI(GATTS_TABLE_TAG,"remote rssi read successful");
			last_rssi_val = param->read_rssi_cmpl.rssi;
			//xEventGroupSetBits(eg,TASK_1_BIT);
		}
		break;
	case ESP_GAP_BLE_PASSKEY_REQ_EVT:	// passkey request event
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GAP_BLE_PASSKEY_REQ_EVT");
		break;
	case ESP_GAP_BLE_OOB_REQ_EVT:	// OOB request event
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
		break;
	case ESP_GAP_BLE_LOCAL_IR_EVT:	// BLE local IR event
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GAP_BLE_LOCAL_IR_EVT");
		break;
	case ESP_GAP_BLE_LOCAL_ER_EVT:	// BLE local ER event
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GAP_BLE_LOCAL_ER_EVT");
		break;
	case ESP_GAP_BLE_NC_REQ_EVT:
		// The app will receive this evt when the IO has DisplayYesNO cap
		// and the peer device IO also has DisplayYesNo capability.
		// show the passkey to user to confirm it with the number on device
		ESP_LOGI(GATTS_TABLE_TAG,
			 "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%d",
			 param->ble_security.key_notif.passkey);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		#ifndef CONFIG_NUMERIC_COMPARISON_EZ_DEV
			blink_out_code(param->ble_security.key_notif.passkey);
        	vTaskDelay(2000 / portTICK_PERIOD_MS);
			//check for button press here
			bool yesno = gpio_get_yesno();
			if(yesno)
            	ESP_LOGI(GATTS_TABLE_TAG, "ESP YES");
			else
            	ESP_LOGI(GATTS_TABLE_TAG, "ESP NO");
			esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, yesno);
		#else
			esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
		#endif
		//esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
		break;
	case ESP_GAP_BLE_SEC_REQ_EVT:
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GAP_BLE_SEC_REQ_EVT");
		/// send the positive(true) security response to the peer device to 
		// accept the security request. If not accept the security request,
		// should sent the security response with negative(false) accept value
		esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
		break;
	case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:	
		//the app will receive this evt when the IO has Output capability and
		// the peer device IO has Input capability. show the passkey number 
		//to the user to input it in the peer deivce.
		ESP_LOGI(GATTS_TABLE_TAG, "The passkey Notify number:%d",
			 param->ble_security.key_notif.passkey);
		blink_out_code(param->ble_security.key_notif.passkey);
		break;
	case ESP_GAP_BLE_KEY_EVT:
		//shows the ble key info share with peer device to the user.
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GAP_BLE_KEY_EVT"); 
		//			esp_key_type_to_str(param->ble_security.ble_key.key_type));
		break;
	case ESP_GAP_BLE_AUTH_CMPL_EVT:
		{
			esp_bd_addr_t bd_addr;
			memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr,
			       sizeof(esp_bd_addr_t));
			ESP_LOGI(GATTS_TABLE_TAG, "remote BD_ADDR: %08x%04x",
				 (bd_addr[0] << 24) + (bd_addr[1] << 16) +
				 (bd_addr[2] << 8) + bd_addr[3],
				 (bd_addr[4] << 8) + bd_addr[5]);
			ESP_LOGI(GATTS_TABLE_TAG, "address type = %d",
				 param->ble_security.auth_cmpl.addr_type);
			ESP_LOGI(GATTS_TABLE_TAG, "pair status = %s",
				 param->ble_security.
				 auth_cmpl.success ? "success" : "fail");
			show_bonded_devices();
			break;
		}
	case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
		{
			ESP_LOGD(GATTS_TABLE_TAG,
				 "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status = %d",
				 param->remove_bond_dev_cmpl.status);
			ESP_LOGI(GATTS_TABLE_TAG,
				 "ESP_GAP_BLE_REMOVE_BOND_DEV");
			ESP_LOGI(GATTS_TABLE_TAG,
				 "-----ESP_GAP_BLE_REMOVE_BOND_DEV----");
			esp_log_buffer_hex(GATTS_TABLE_TAG,
					   (void *)param->
					   remove_bond_dev_cmpl.bd_addr,
					   sizeof(esp_bd_addr_t));
			ESP_LOGI(GATTS_TABLE_TAG,
				 "------------------------------------");
			break;
		}
	case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
		ESP_LOGI(GATTS_TABLE_TAG,
			 "ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT");
		if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS) {
			ESP_LOGE(GATTS_TABLE_TAG,
				 "config local privacy failed, error status = %x",
				 param->local_privacy_cmpl.status);
			break;
		}
#ifdef CONFIG_SET_RAW_ADV_DATA
		esp_err_t raw_adv_ret =
		    esp_ble_gap_config_adv_data_raw(raw_adv_data,
						    sizeof(raw_adv_data));
		if (raw_adv_ret) {
			ESP_LOGE(GATTS_TABLE_TAG,
				 "config raw adv data failed, error code = %x ",
				 raw_adv_ret);
		}
		adv_config_done |= ADV_CONFIG_FLAG;
		esp_err_t raw_scan_ret =
		    esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data,
							 sizeof
							 (raw_scan_rsp_data));
		if (raw_scan_ret) {
			ESP_LOGE(GATTS_TABLE_TAG,
				 "config raw scan rsp data failed, error code = %x",
				 raw_scan_ret);
		}
		adv_config_done |= SCAN_RSP_CONFIG_FLAG;
#else
		//config adv data
		esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
		if (ret) {
			ESP_LOGE(GATTS_TABLE_TAG,
				 "config adv data failed, error code = %x",
				 ret);
		}
		adv_config_done |= ADV_CONFIG_FLAG;
		//config scan response data
		ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
		if (ret) {
			ESP_LOGE(GATTS_TABLE_TAG,
				 "config scan response data failed, error code = %x",
				 ret);
		}
		adv_config_done |= SCAN_RSP_CONFIG_FLAG;
#endif
		break;

	default:
		break;
	}
}

void set_device_value(uint16_t write_handle, uint8_t write_val)
{
	if(write_handle == m_handles[IDX_CHAR_VAL_A]) {
		ESP_LOGI(GATTS_TABLE_TAG, "buzz");
		if (write_val == 0x00) {
			gpio_set_level(GPIO_OUTPUT_IO_0, 0);
			gpio_white_off();
		} else if (write_val == 0x01) {
			gpio_set_level(GPIO_OUTPUT_IO_0, 1);
			gpio_white_on();
		} else {
			ESP_LOGE(GATTS_TABLE_TAG, "invalid value. need 1 or 0.");
		}
	}
	else if(write_handle == m_handles[IDX_CHAR_VAL_B]) {
		if (write_val == 0x00) {
			ESP_LOGI(GATTS_TABLE_TAG, "mode: cancel all !!");
			if(mode_internal != MODE_LOOP)
				return; //can only enter idle state from loop state

			mode_internal = MODE_IDLE;
			//stop characteristic handler loop
			vTaskSuspend( check_chars_thandle  );

		} else if (write_val == 0x01) {
			ESP_LOGI(GATTS_TABLE_TAG, "mode: fw update");
			if(mode_internal == MODE_FWUPDATE)
				return; //don't enter update state twice

			if(mode_internal == MODE_LOOP)
				vTaskSuspend( check_chars_thandle  ); 

			mode_internal = MODE_FWUPDATE;
			//stop characteristic handler loop
			//vTaskSuspend( check_chars_thandle  );
			//init wifi, AP, socket server, recv, write to partition
			wifi_connect_init(int_pass);
			socket_server(); //timeout
			wifi_connect_destroy();
			//restart
			vTaskDelay(2000 / portTICK_RATE_MS);
			esp_restart();
		} else if (write_val == 0x02) {
			ESP_LOGI(GATTS_TABLE_TAG, "mode: check char loop");
			if(mode_internal != MODE_IDLE)
				return; //only idle state can enter loop state
			
			mode_internal = MODE_LOOP;
			mode_loop = LOOP_NONE;
			xTaskCreate(check_char_thresholds_task,"char task loop", 8192, 
												NULL, 1, &check_chars_thandle);
		} else {
			ESP_LOGE(GATTS_TABLE_TAG, "Invalid mode. Enter 0, 1, 2");
		}
	}
	else if(write_handle == m_handles[IDX_CHAR_VAL_C]) {
		ESP_LOGI(GATTS_TABLE_TAG, "fixed int !!");
		//change internal mode state
		mode_loop = LOOP_FIXEDINT;
		threshold_val = write_val;
		//parameter check?
	}
	else if(write_handle == m_handles[IDX_CHAR_VAL_D]) {
		ESP_LOGI(GATTS_TABLE_TAG, "random int !!");
		//change internal mode state
		mode_loop = LOOP_RANDINT;
		threshold_val = write_val;
		uint16_t rand_index;
		for(int j = 0; j < 3600; j++) {
			rand_array[j] = 0;
		}
		for(int k = 0; k < threshold_val; k++) {
			rand_index = esp_random() % 3600;
			if(rand_array[rand_index] != 1) {
				rand_array[rand_index] = 1;
			} else {
				k--;
			}
		}
		ESP_LOGI(GATTS_TABLE_TAG, "random beep schedule:");
		for(int j = 0; j < 3600; j++) {
			if(rand_array[j] == 1) {
				ESP_LOGI(GATTS_TABLE_TAG, "> %d", j);
			}
		}
	}
	else if(write_handle == m_handles[IDX_CHAR_VAL_F]) {
		ESP_LOGI(GATTS_TABLE_TAG, "rssi min !!");
		//change internal mode state
		mode_loop = LOOP_RSSI;
		threshold_val = write_val;
	}
	else if(write_handle == m_handles[IDX_CHAR_VAL_I]) {
		ESP_LOGI(GATTS_TABLE_TAG, "solar min !!");
		//change internal mode state
		mode_loop = LOOP_SOLAR;
		threshold_val = write_val;
	}
	else {
		ESP_LOGI(GATTS_TABLE_TAG, "can't write that handle!");
	}
}

void
gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
											esp_ble_gatts_cb_param_t * param)
{
	switch (event) {
	case ESP_GATTS_REG_EVT:
	{
		esp_err_t name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
		if (name_ret) {
			ESP_LOGE(GATTS_TABLE_TAG,"ESP_GATTS_REG_EVT error %x", name_ret);
		}
		//generate a resolvable random address ..
		esp_ble_gap_config_local_privacy(true);

		esp_err_t attr_ret = 
		esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, IDX_NB, SVC_INST_ID);
		if (attr_ret) {
			ESP_LOGE(GATTS_TABLE_TAG, "attr table failed: %x",attr_ret);
		}
		break;
	}
	case ESP_GATTS_READ_EVT:
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");
		break;
	case ESP_GATTS_WRITE_EVT:
		if (!param->write.is_prep) {
			// length of gattc write data must be < GATTS_CHAR_VAL_LEN_MAX.
			ESP_LOGI(GATTS_TABLE_TAG,
						"GATT_WRITE_EVT, handle = %d, value len = %d, value :",
				 						param->write.handle, param->write.len);
			esp_log_buffer_hex(
						GATTS_TABLE_TAG, param->write.value, param->write.len);

			//all vals must be 1 byte long
			set_device_value(param->write.handle, param->write.value[0]);

			// send response when param->write.need_rsp is true
			if (param->write.need_rsp) {
				esp_ble_gatts_send_response(gatts_if,param->write.conn_id,
							    	param->write.trans_id,ESP_GATT_OK, NULL);
			}
		} else {
			ESP_LOGI(GATTS_TABLE_TAG, "bad code path ");
		}
		break;
	case ESP_GATTS_EXEC_WRITE_EVT:
		// length of gattc prapare write data must be < GATTS_CHAR_VAL_LEN_MAX 
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT ?");
		//exec_write_event_env(&prepare_write_env, param);
		break;
	case ESP_GATTS_MTU_EVT:
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_MTU_EVT, MTU %d",
			 param->mtu.mtu);
		break;
	case ESP_GATTS_CONF_EVT:
		//ESP_LOGI(GATTS_TABLE_TAG,"ESP_GATTS_CONF_EVT %d",param->conf.status);
		break;
	case ESP_GATTS_START_EVT:
		ESP_LOGI(GATTS_TABLE_TAG, "SERVICE_START_EVT, status %d, handle %d",
			 param->start.status, param->start.service_handle);
		break;
	case ESP_GATTS_CONNECT_EVT:
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d",
													 param->connect.conn_id);
		esp_log_buffer_hex(GATTS_TABLE_TAG, param->connect.remote_bda, 6);
		memcpy(conn_params.bda, param->connect.remote_bda, 
														sizeof(esp_bd_addr_t));

		//reference apple documentation for ios ble conn param restrictions
		conn_params.latency = 0;
		conn_params.max_int = 0x20;	// max_int = 0x20*1.25ms = 40ms
		conn_params.min_int = 0x10;	// min_int = 0x10*1.25ms = 20ms
		conn_params.timeout = 400;	// timeout = 400*10ms = 4000ms
		//start sent the update connection parameters to the peer device.
		connected = true;
        show_bonded_devices();
        //needed? 
		//esp_ble_gap_update_conn_params(&conn_params);
		break;
	case ESP_GATTS_DISCONNECT_EVT:
		ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d",
													 param->disconnect.reason);
		connected = false;
		//mode_internal = MODE_IDLE;
		////vTaskSuspend( check_chars_thandle );
		esp_ble_gap_start_advertising(&adv_params);
		break;
	case ESP_GATTS_CREAT_ATTR_TAB_EVT:
	{
		if (param->add_attr_tab.status != ESP_GATT_OK) {
			ESP_LOGE(GATTS_TABLE_TAG, "attr table failed error= 0x%x",
					 							param->add_attr_tab.status);
		} else if (param->add_attr_tab.num_handle != IDX_NB) {
			ESP_LOGE(GATTS_TABLE_TAG, "attr table abnormal, num_handle (%d) \
			not equal to IDX_NB(%d)",param->add_attr_tab.num_handle, IDX_NB);
		} else {
			ESP_LOGI(GATTS_TABLE_TAG, "attr table success, handle = %d\n",
					 						param->add_attr_tab.num_handle);
			memcpy(m_handles, param->add_attr_tab.handles, sizeof(m_handles));
			esp_ble_gatts_start_service(m_handles[IDX_SVC]);
		}
		break;
	}
	case ESP_GATTS_STOP_EVT:
	case ESP_GATTS_OPEN_EVT:
	case ESP_GATTS_CANCEL_OPEN_EVT:
	case ESP_GATTS_CLOSE_EVT:
	case ESP_GATTS_LISTEN_EVT:
	case ESP_GATTS_CONGEST_EVT:
	case ESP_GATTS_UNREG_EVT:
	case ESP_GATTS_DELETE_EVT:
	default:
		break;
	}
}

void
gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
		    esp_ble_gatts_cb_param_t * param)
{
	// If event is register event, store the gatts_if for each profile
	if (event == ESP_GATTS_REG_EVT) {
		if (param->reg.status == ESP_GATT_OK) {
			m_profile_tab[PROFILE_IDX].gatts_if = gatts_if;
		} else {
			ESP_LOGE(GATTS_TABLE_TAG, "reg app failed, app_id %04x, status %d",
				 	param->reg.app_id, param->reg.status);
			return;
		}
	}
	esp_gatt_if_t curr_gatts_if = m_profile_tab[0].gatts_if;
	if (gatts_if == ESP_GATT_IF_NONE || gatts_if == curr_gatts_if) {
		if (m_profile_tab[0].gatts_cb) {
			m_profile_tab[0].gatts_cb(event, gatts_if, param);
		}
	}
}

int get_rssi()
{
	// We make the API call to read the RSSI value which is an async operation
	// receiving ESP_GAP_BLE_READ_RSSI_COMPLETE_EVT indicates completion
	esp_err_t rc = esp_ble_gap_read_rssi(conn_params.bda);
	if (rc != ESP_OK) {
		ESP_LOGE(LOG_TAG, "<< getRssi: esp_ble_gap_read_rssi: rc=%d", rc);
		return 0;
	}

	return last_rssi_val;
}

//read sensor data and beep depending on mode and thresholds or schedule
void check_char_thresholds_task()
{
	int rssi, solar;
	uint8_t notify_data2[1];
	uint8_t notify_data[1];
	uint16_t secs = 0;
	for (;;) {
		secs = (secs + 1) % 3600;
		if (connected) {

			rssi = get_rssi();
			notify_data[0] = (float)(rssi + 127)/147 * 100;
			esp_ble_gatts_set_attr_value(m_handles
						     [IDX_CHAR_VAL_E], 0x1, notify_data);
			esp_ble_gatts_send_indicate(m_profile_tab[PROFILE_IDX].gatts_if,
						    m_profile_tab[PROFILE_IDX].conn_id,
						    m_handles[IDX_CHAR_VAL_E],
						    sizeof(notify_data), notify_data, false);

			solar = gpio_sample_adc();
			notify_data2[0] = (float)solar/4095 * 100;
			//notify_data2[1] = (solar >> 8) & 0xFF;
			esp_ble_gatts_set_attr_value(m_handles[IDX_CHAR_VAL_H], 0x1,
						     notify_data2);
			esp_ble_gatts_send_indicate(m_profile_tab[PROFILE_IDX].gatts_if,
						    m_profile_tab[PROFILE_IDX].conn_id,
						    m_handles[IDX_CHAR_VAL_H],
						    sizeof(notify_data2), notify_data2, false);

			switch(mode_loop) {
				case LOOP_FIXEDINT:
					if((secs % (threshold_val * 60)) == 0) { short_beep(); }
					break;
				case LOOP_RANDINT:
					if(rand_array[secs]==1) { short_beep(); }
					break;
				case LOOP_SOLAR:
					if(notify_data2[0] > threshold_val) { short_beep(); }
					break;
				case LOOP_RSSI:
					if(notify_data[0] > threshold_val) { short_beep(); }
					break;
				default:
					gpio_set_level(GPIO_OUTPUT_IO_0, 0);
					break;
			}
		
			if(secs % 5 == 0) {
				gpio_blink_one(GPIO_GREEN);
				ESP_LOGI(GATTS_TABLE_TAG,"solar %d rssi %d \n", solar, rssi);
			}
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

//OTA update is over WiFi; make a random password for the AP and store
// in encrypted attribute to be read later. regenerated every reboot
void generate_pass_char()
{
	char pass_str[9];
	uint8_t notify_pass[8];
	int_pass =  (uint64_t) esp_random() << 32 | esp_random() ;
	for(int i = 0; i < 8; i++) {
		notify_pass[i] = ((((int_pass >> 8*i) & 0xFF) % 94) + 33);
		pass_str[i] = notify_pass[i];
	}
	pass_str[8] = '\0';
	ESP_LOGI(GATTS_TABLE_TAG,"--gencred--)> %s",pass_str);

	esp_ble_gatts_set_attr_value(m_handles[IDX_CHAR_VAL_J], 0x8, notify_pass);
}

//set up BLE controller and security paramters
void gatts_ble_init()
{
	esp_err_t ret;

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s",
			 __func__, esp_err_to_name(ret));
		return;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s",
			 __func__, esp_err_to_name(ret));
		return;
	}

	ret = esp_bluedroid_init();
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s",
			 __func__, esp_err_to_name(ret));
		return;
	}

	ret = esp_bluedroid_enable();
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s",
			 __func__, esp_err_to_name(ret));
		return;
	}

	ret = esp_ble_gatts_register_callback(gatts_event_handler);
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "gatts register error = %x", ret);
		return;
	}

	ret = esp_ble_gap_register_callback(gap_event_handler);
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "gap register error = %x", ret);
		return;
	}

	ret = esp_ble_gatts_app_register(ESP_APP_ID);
	if (ret) {
		ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error = %x", ret);
		return;
	}

	//esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
	//if (local_mtu_ret){
	//	ESP_LOGE(GATTS_TABLE_TAG, "MTU failed, err code = %x", local_mtu_ret);
	//}

	#ifdef CONFIG_BLE_GATTS_SECURITY
		// set the security iocap & auth_req & key size & init key response key 
		// parameters to the stack. set to bond with peer after auth
		esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
		// bonding with peer device after authentication
		// esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_BOND;	
		esp_ble_io_cap_t iocap = ESP_IO_CAP_IO;
	#else
		esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_ONLY;
		esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
	#endif
	uint8_t key_size = 16;	//the key size should be 7~16 bytes
	uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, 8);
	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, 8);
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, 8);
	// If your BLE device acts as Slave, the init_key means you hope which 
	// types of key of the master should distribute to you, and the response
	// key means which key you can distribute to the Master; If your BLE device
	// act as a master, the response key means you hope which types of key of
	// the slave should distribut to you, and the init key means which key
	// you can distribut to the slave.
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, 8);
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, 8);

	generate_pass_char();
	#ifdef CONFIG_BLE_GATTS_SECURITY 
		ESP_LOGI(GATTS_TABLE_TAG,"v0.1.e");
	#else
		ESP_LOGI(GATTS_TABLE_TAG,"v0.1.a");
	#endif
		
}

