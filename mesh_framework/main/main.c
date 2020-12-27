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

#include <cJSON.h>

void app_main(void) {
	meshf_init();
	meshf_start();
	meshf_mqtt_start();
	uint8_t rx_mensagem[180] = {0,};
	meshf_rx(rx_mensagem);

	meshf_asktime();
	meshf_mqtt_publish("/data/esp32",strlen("/data/esp32"),"string",strlen("string"));
	char mac_destination[] = "80:7D:3A:B7:C8:18";
	meshf_sleep_time(1000);
	meshf_ping(mac_destination);
}