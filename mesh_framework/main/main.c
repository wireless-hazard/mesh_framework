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
	uint8_t mensagem[] = {10,25};
	uint8_t rx_mensagem[2] = {0,};
	char mac[] = "A4:CF:12:75:21:31"; //softAP
	meshf_init();
	meshf_start();
	meshf_rx(rx_mensagem);
	meshf_tx_p2p(
			mac,
			mensagem,
			sizeof(mensagem));
	meshf_task_debugger();
	data_ready();
	printf("\n");
	for(int i = 0;i < sizeof(rx_mensagem);i++){
		printf(" %d ",rx_mensagem[i]);
	}
	free_rx_buffer();
	meshf_task_debugger();
}