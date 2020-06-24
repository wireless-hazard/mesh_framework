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
	uint8_t mensagem[2] = {0,};
	meshf_init(); //Inicializa as configuracoes
	meshf_start(); //Inicializa a rede MESH
	int i = 0;
	printf("\n");
	while(true){
		mensagem[1] = i;
		meshf_tx_TODS(
			"192.168.0.6",
			8000,
			mensagem,
			sizeof(mensagem));
		i++;
		meshf_sleep_time(10000);
	}
}