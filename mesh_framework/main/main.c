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
	uint8_t mensagem[] = {100,20,4,7};
	uint8_t rx_mensagem[4] = {0,};
	char mac[] = "A4:CF:12:75:21:31";
	meshf_init();
	meshf_start();
	meshf_rx(rx_mensagem);
	meshf_tx_p2p(
		mac,
		mensagem,
		sizeof(mensagem));
	while (!data_ready()){
		vTaskDelay(1*1000/portTICK_PERIOD_MS);
	}
	for(int i = 0;i < sizeof(rx_mensagem);i++){
		printf("%d\n",rx_mensagem[i]);
	}	
}