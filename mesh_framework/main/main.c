#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_mesh.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include <esp_system.h>
#include <time.h>
#include <sys/time.h>
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"
#include "mesh_framework.h"

#include <driver/adc.h>
#include "driver/gpio.h"
#include "esp_sntp.h"
#include "esp_sleep.h"
#include <cJSON.h>

void app_main(void) {
	gpio_reset_pin(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    gpio_set_level(2, 1);

    char mqtt_data[150];
    char strftime_buff[64];
	time_t now = 0;
    struct tm timeinfo = { 0 }; 
    uint8_t self_mac[6] = {0,};
    
	meshf_init();
	meshf_start(true,true);
	uint8_t rx_mensagem[180] = {0,};
	meshf_rx(rx_mensagem);
	int resp = 0;
	meshf_asktime();
	time(&now);
    localtime_r(&now, &timeinfo);
	strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);

	esp_read_mac(self_mac,ESP_MAC_WIFI_SOFTAP);

	sprintf(mqtt_data, "The current date/time in %02x:%02x:%02x:%02x:%02x:%02x is: %s", self_mac[0],self_mac[1],self_mac[2],self_mac[3],self_mac[4],self_mac[5],strftime_buff);
	resp = meshf_mqtt_publish("/data/esp32",strlen("/data/esp32"),mqtt_data,strlen(mqtt_data));
	printf("%s\n",esp_err_to_name(resp));

    meshf_sleep_time(1000);
	// esp_mesh_stop();
	esp_wifi_stop();
	esp_deep_sleep(300000000);
	// char mac_destination[] = "80:7D:3A:B7:C8:18";
	// meshf_sleep_time(1000);
	// meshf_ping(mac_destination);
}