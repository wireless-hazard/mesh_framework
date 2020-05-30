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

#define MAX_LAYERS CONFIG_MAX_LAYERS
#define WIFI_CHANNEL 0
#define ROUTER_SSID CONFIG_ROUTER_SSID
#define ROUTER_PASSWORD CONFIG_ROUTER_PASSWORD
#define MAX_CLIENTS CONFIG_MAX_CLIENTS
 
static const uint8_t MESH_ID[6] = {0x05, 0x02, 0x96, 0x05, 0x02, 0x96};

void mesh_event_handler(mesh_event_t evento){
	
}

void meshf_init(){

	//Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

	tcpip_adapter_init(); //Inicializa as estruturas de dados TCP/LwIP e cria a tarefa principal LwIP
		
	ESP_ERROR_CHECK(esp_event_loop_init(NULL,NULL)); //Lida com os eventos AINDA NAO IMPLEMENTADOS
	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //
	esp_wifi_init(&cfg); //Inicia o Wifi com os seus parametros padroes
	
	ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));//Desliga o cliente dhcp
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));//Desliga o servidor dhcp
	
	ESP_ERROR_CHECK(esp_wifi_start());

	// Inicializacao Do MESH
	
	ESP_ERROR_CHECK(esp_mesh_init()); //Inicializa o "mesh stack"
	ESP_ERROR_CHECK(esp_mesh_set_max_layer(MAX_LAYERS));//Numero maximo de niveis da rede
	ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(0.9));//Porcentagem minima para a escolha do Noh raiz
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(40));//Tempo sem comunicacao entre pai e filho com que fara a dissociacao do filho
	
	mesh_cfg_t config_mesh = MESH_INIT_CONFIG_DEFAULT(); //Possui a configuracao base mesh para aplicar depois
	
	//Mesh Network Identifier (MID)
	memcpy((uint8_t *) &config_mesh.mesh_id,MESH_ID,6);
	
	config_mesh.event_cb = &mesh_event_handler; //mesh event callback
	config_mesh.channel = WIFI_CHANNEL;
	config_mesh.router.ssid_len = strlen(ROUTER_SSID);
	memcpy((uint8_t *) &config_mesh.router.ssid, ROUTER_SSID, config_mesh.router.ssid_len);
    memcpy((uint8_t *) &config_mesh.router.password, ROUTER_PASSWORD, strlen(ROUTER_PASSWORD));
	config_mesh.mesh_ap.max_connection = MAX_CLIENTS;
    memcpy((uint8_t *) &config_mesh.mesh_ap.password, ROUTER_PASSWORD, strlen(ROUTER_PASSWORD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&config_mesh));
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());
}