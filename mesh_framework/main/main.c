#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
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
	
	uint8_t rx_mensagem[20] = {0,};
	meshf_rx(rx_mensagem);

	char mac_destination[] = "80:7D:3A:B7:C8:18";
	meshf_ping(mac_destination);
	
}