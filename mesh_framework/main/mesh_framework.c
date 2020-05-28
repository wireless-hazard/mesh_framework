#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "mesh_framework.h"

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN
#define WIFI_MODE_SET 			   CONFIG_AP_MODE

static const char *TAG = "wifi softAP";

esp_err_t event_handler(void *ctx, system_event_t *event){
    switch(event->event_id) {
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void meshf_init(){ 
	//Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    if (strcmp(WIFI_MODE_SET,"STA") == 0){
    	wifi_config_t sta_config = {
        	.sta = {
            	.ssid = CONFIG_ESP_WIFI_SSID,
            	.password = CONFIG_ESP_WIFI_PASSWORD,
            	.bssid_set = false
        	}
    	};
    	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    	ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    	ESP_ERROR_CHECK( esp_wifi_start() );
    	ESP_ERROR_CHECK( esp_wifi_connect() );
    }else{
    	wifi_config_t wifi_config = {
        	.ap = {
            	.ssid = EXAMPLE_ESP_WIFI_SSID,
            	.ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            	.password = EXAMPLE_ESP_WIFI_PASS,
            	.max_connection = EXAMPLE_MAX_STA_CONN,
            	.authmode = WIFI_AUTH_WPA_WPA2_PSK
        	},
    	};
    	if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        	wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    	}

    	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    	ESP_ERROR_CHECK(esp_wifi_start());
    }
}

void meshf_connect(){
	ESP_LOGI(TAG,"%s",WIFI_MODE_SET);
}