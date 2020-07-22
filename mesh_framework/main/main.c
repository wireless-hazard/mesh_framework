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
#include "lwip/sockets.h"
#include "mesh_framework.h"

void app_main(void) {
	meshf_init(); //Inicializa as configuracoes
	meshf_start(); //Inicializa a rede MESH
	uint8_t buffer[2] = {0,};
	while(true){
		buffer[0] = (uint8_t)esp_random();
		buffer[1] = (uint8_t)esp_random();
		meshf_tx_TODS(
			"192.168.0.2",
			8000,
			buffer,
			sizeof(buffer));
		meshf_sleep_time(5000);
	}
}