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
	int8_t rssi;
	uint8_t mensagem[3] = {192,168,0}; // Mensagem a ser transmitida
	char mac[] = "48:83:C7:94:54:3E"; //MAC para o qual a mensagem sera transmitida
	meshf_init(); //Inicializa as configuracoes
	meshf_start(); //Inicializa a rede MESH
	
	printf("\n");
	while(true){
		meshf_rssi_info(&rssi,mac);
		meshf_sleep_time(10000);
	}
}