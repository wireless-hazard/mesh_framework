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
	uint8_t mensagem[] = {234,5,4,7};
	char mac[] = "00:AA:00:00:00:00";
	meshf_init();
	meshf_tx_p2p(
		mac,
		mensagem,
		sizeof(mensagem));
	meshf_start();
}