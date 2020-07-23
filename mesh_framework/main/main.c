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
#include "lwip/sockets.h"
#include "mesh_framework.h"

void app_main(void) {
	uint8_t rx_buffer[5] = {0,};
	meshf_init();
	meshf_start();
	meshf_rx(rx_buffer);
}