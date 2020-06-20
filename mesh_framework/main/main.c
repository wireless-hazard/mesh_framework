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
	uint8_t mensagem[] = {15,20,4,7,25,37}; // Mensagem a ser transmitida
	uint8_t rx_mensagem[6] = {0,}; //Buffer que recebera a mensagem recebida
	char mac[] = "A4:CF:12:75:21:31"; //MAC para o qual a mensagem sera transmitida
	meshf_init(); //Inicializa as configuracoes
	meshf_start(); //Inicializa a rede MESH
	meshf_rx(rx_mensagem); //Inicializa o receptor das mensagens
	meshf_tx_p2p(
		mac,
		mensagem,
		sizeof(mensagem)); //Transmite a mensagem {100,20,4,7} para "A4:CF:12:75:21:31"
	while (!data_ready()){ //Enquanto os dados nao estiverem prontos...
		meshf_sleep_time(1000); //...insere um delay de 1 segundo
	}
	printf("\n");
	for(int i = 0;i < sizeof(rx_mensagem);i++){ //Printa a mensagem no console
		printf(" %d ",rx_mensagem[i]);
	}
	printf("\n");
	free_rx_buffer(); //Libera o buffer
	while(true){
		meshf_tx_TODS( //Manda mensagem para o IP
				"192.168.0.6",
				8000,
				mensagem,
				sizeof(mensagem));
		meshf_sleep_time(5000);
	}
		
}