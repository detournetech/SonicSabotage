#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "wifi_connect.h"
#include "iap.h"

// FreeRTOS event group to signal when we are connected
static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "wifi connect";

int recv_fw(int client_sock) 
{
    iap_err_t result;
	ESP_LOGI(TAG, "- IAP (In-App Programming) init ");

	// setup space for new client data
	int chunk = 10 * 1024;
	int size_used = 0;
	char *data = malloc(chunk);
    char *data_len = malloc(8);

	// recv length of fw bin
	ssize_t size_read = recv(client_sock, data_len, 8, 0);
	if (size_read != 8) {
		ESP_LOGE(TAG, "recv: %d %s", size_read, strerror(errno));
		return -1;
	}

    uint64_t ilen = 
		data_len[7] | (data_len[6]<<8) | (data_len[5]<<16) | (data_len[4]<<24)|
		((uint64_t)data_len[3] << 32) | ((uint64_t)data_len[2] << 40) |
		((uint64_t)data_len[1] << 48) | ((uint64_t)data_len[0] << 56);
	ESP_LOGI(TAG, "- firmware size (bytes): %" PRIu64 "\n", ilen);

	#ifdef CONFIG_OTA_CAN_WRITE_FLASH
		iap_init();
    	result = iap_begin();
    	if (result == IAP_ERR_SESSION_ALREADY_OPEN) {
        	iap_abort();
        	result = iap_begin();
    	}

    	if (result != IAP_OK) {
        	ESP_LOGE(TAG, "iap_begin failed (%d)!", result);
        	return -1;
    	}
	#endif

	ESP_LOGI(TAG, "- begin receiving fw data");
	while (1) {
		ssize_t size_read = recv(client_sock, data, ilen-size_used, 0);
		if (size_read < 0) {
			ESP_LOGE(TAG, "recv: %d %s", size_read, strerror(errno));
			return -1;
		}
		if (size_read == 0) { break; }
		size_used += size_read;

		#ifdef CONFIG_OTA_CAN_WRITE_FLASH
			// Write the received data to the flash.
			result = iap_write((uint8_t*)data, size_read);
			if (result != IAP_OK) {
				ESP_LOGE(TAG, "iap_write failed (%d), abort fw update!", result);
				iap_abort();
				return -1;
			}
		#endif
	}

	ESP_LOGI(TAG, "- data written (bytes): %d", size_used);
	#ifdef CONFIG_OTA_CAN_WRITE_FLASH
		result = iap_commit();
    	if (result != IAP_OK) {
        	ESP_LOGE(TAG, "iap: closing the session has failed (%d)!", result);
			return -1;
    	}
	#endif
	free(data);
	return 0;
}

//tcp socket server. receives fw bin on connection and writes it to flash
void socket_server()
{
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	ESP_LOGI(TAG, " - socket");
	// Create a socket that we will listen upon.
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		ESP_LOGE(TAG, "socket: %d %s", sock, strerror(errno));
		return;
	}

	ESP_LOGI(TAG, "- bind");
	// Bind our server socket to a port.
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT_NUMBER);
	int rc = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (rc < 0) {
		ESP_LOGE(TAG, "bind: %d %s", rc, strerror(errno));
		return;
	}

	ESP_LOGI(TAG, "- listen");
	// Flag the socket as listening for new connections.
	rc = listen(sock, 5);
	if (rc < 0) {
		ESP_LOGE(TAG, "listen: %d %s", rc, strerror(errno));
		return;
	}

	ESP_LOGI(TAG, "- accept");
	// Listen for a new client connection.
	socklen_t client_addr_len = sizeof(client_addr);
	int client_sock = accept(sock, (struct sockaddr *)&client_addr, 
		&client_addr_len);

	if (client_sock < 0) {
		ESP_LOGE(TAG, "accept: %d %s", client_sock, strerror(errno));
		return;
	}

	ESP_LOGI(TAG, "- accept");
	recv_fw(client_sock);

	ESP_LOGI(TAG, "- close");
	close(client_sock);
}

static esp_err_t event_handler(void *ctx, system_event_t * event)
{
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(TAG, "got ip:%s",
			 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG, "station:" MACSTR " join, AID=%d",
			 MAC2STR(event->event_info.sta_connected.mac),
			 event->event_info.sta_connected.aid);
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",
			 MAC2STR(event->event_info.sta_disconnected.mac),
			 event->event_info.sta_disconnected.aid);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}

void wifi_connect_init(uint64_t pass_int)
{
	wifi_event_group = xEventGroupCreate();

	char pass_str[9];// = (char*)malloc(9);
    for(int i = 0; i < 8; i++) {
      pass_str[i] = ((((pass_int >> 8*i) & 0xFF) % 94) + 33);
    }   
    pass_str[8] = '\0';

	tcpip_adapter_init();
	ESP_LOGI(TAG, "- TCP adapter initialized\n");

	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	ESP_LOGI(TAG, "- DHCP server stopped\n");

	// assign a static IP to the network interface
	tcpip_adapter_ip_info_t info;
	memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 1, 1);
	IP4_ADDR(&info.gw, 192, 168, 1, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	ESP_LOGI(TAG, "- TCP adapter configured with IP 192.168.1.1/24\n");

	// start the DHCP server   
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	ESP_LOGI(TAG, "- DHCP server started\n");

	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	wifi_config_t wifi_config = {
		.ap = {
		       .ssid = DTC_ESP_WIFI_SSID,
		       .ssid_len = strlen(DTC_ESP_WIFI_SSID),
		       .password = DTC_ESP_WIFI_PASS,
		       .max_connection = DTC_MAX_STA_CONN,
		       .authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	}; 

    memcpy(wifi_config.ap.password, pass_str, 8);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
		 DTC_ESP_WIFI_SSID, pass_str);
}

void wifi_connect_destroy()
{
	ESP_ERROR_CHECK(esp_wifi_stop());
}
