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
static const char *MESH_TAG = "mesh_tagger";
static bool is_mesh_connected = false;
static int mesh_layer = -1;
static mesh_addr_t mesh_parent_addr;
static bool is_parent_connected = false;

static uint8_t tx_buffer[1472] = { 0, };
static mesh_addr_t tx_destination;

void STR2MAC(char rec_string[17],uint8_t *address){
	uint8_t mac[6] = {0,};
	int j = 0;
	for (int i = 0;i<=17;i+=3){
		if((uint8_t)rec_string[i] <= 58){
			mac[j] = (((uint8_t)rec_string[i]-48) * 16) + ((uint8_t)rec_string[i+1]-48);
		}else{
			mac[j] = (((uint8_t)rec_string[i]-55) * 16) + ((uint8_t)rec_string[i+1]-55);
		}
		j++;
	}
	memcpy(address,&mac,sizeof(uint8_t)*6);

}

void tx_p2p(void *pvParameters){
	
	mesh_data_t tx_data;
	int flag = 0;

	while(true){
		if (is_parent_connected){
			ESP_LOGE(MESH_TAG,"%d\n",tx_buffer[2]);
			vTaskDelay(1000/portTICK_PERIOD_MS);
		}
	}
}

void meshf_tx_p2p(char mac_destination[], uint8_t transmitted_data[], uint16_t data_size){
	
	memcpy(tx_buffer,transmitted_data,data_size);
	STR2MAC(mac_destination,&tx_destination.addr);
	xTaskCreatePinnedToCore(&tx_p2p,"P2P transmission",4096,NULL,5,NULL,1);
}

void mesh_event_handler(mesh_event_t evento){
	switch (evento.id){
		static int filhos = 0;
		static uint8_t last_layer = 0;
		//
		//MESH MAIN SETUP EVENTS
		//
		case MESH_EVENT_STARTED: //start sending Wifi beacon frames and begin scanning for preferred parent.	
			is_mesh_connected = false;
			mesh_layer = esp_mesh_get_layer();
			ESP_LOGW(MESH_TAG,"MESH STARTED\n");

		break;
		case MESH_EVENT_STOPPED: //Reset the mesh stack's status on the device
			is_mesh_connected = false;
        	mesh_layer = esp_mesh_get_layer();
			ESP_LOGE(MESH_TAG,"MESH_EVENT_STOPPED\n");
		break;
		case MESH_EVENT_PARENT_CONNECTED:
			//COPY AND PAST'ED 
			mesh_layer = evento.info.connected.self_layer;
			memcpy(&mesh_parent_addr.addr, evento.info.connected.connected.bssid, 6);
			last_layer = mesh_layer;
        	is_mesh_connected = true;
        	is_parent_connected = true;
			if (esp_mesh_is_root()) {
            	tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        	}
    		ESP_LOGE(MESH_TAG,"MESH_EVENT_PARENT_CONNECTED\n");
    		
		break;
		case MESH_EVENT_PARENT_DISCONNECTED: //Perform a fixed number of attempts to reconnect before searching for another one
			is_mesh_connected = false;
        	mesh_layer = esp_mesh_get_layer();
			ESP_LOGW(MESH_TAG,"MESH_EVENT_PARENT_DISCONNECTED\n");
		break;
		case MESH_EVENT_NO_PARENT_FOUND:
			ESP_LOGW(MESH_TAG,"eternamente IDLE\nESP32 ira reiniciar\n");
			esp_restart();

		break;
		//
		//MESH NETWORK UPDATE EVENTS
		//
		case MESH_EVENT_CHILD_CONNECTED: //A child node sucessfully connects  to the  node's  SoftAP interface
			filhos = filhos + 1;
			ESP_LOGW(MESH_TAG,"MESH_EVENT_CHILD_CONNECTED %d\n",filhos);
		break;
		case MESH_EVENT_CHILD_DISCONNECTED: //A child node disconnects from  a node 
			filhos = filhos - 1;
			printf(MAC2STR(evento.info.child_disconnected.mac));
		break;
		case MESH_EVENT_ROUTING_TABLE_ADD: //A node's descendant (with its possible futher descendants) joins the mesh network
			ESP_LOGW(MESH_TAG,"MAC adicionado a tabela de roteamento\n");
		break;
		case MESH_EVENT_ROUTING_TABLE_REMOVE: //A node's descendant (with its possible futher descendants) desconnects from the mesh network
			ESP_LOGW(MESH_TAG,"MAC removido da tabela de roteamento\n");
		break;
		case MESH_EVENT_ROOT_ADDRESS: //Propagacao do MAC do noh raiz da rede Mesh
			ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_ADDRESS\n");		
		break;
		case MESH_EVENT_ROOT_FIXED: //Quando a configuracao de root fixo difere entre dois nohs tentando comunicar
			ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_FIXED ISSO NAO PODE ACONTECER\n");
		break;
		case MESH_EVENT_TODS_STATE: //The node is informed of a change in the accessibility of the external DS
			ESP_LOGW(MESH_TAG,"MESH_EVENT_TODS_STATE\n");
		break;
		case MESH_EVENT_VOTE_STARTED: //A new rote election is started in the mesh network
			ESP_LOGW(MESH_TAG,"Votacao comecou\n");
		break;
		case MESH_EVENT_VOTE_STOPPED: //The election has ended
			ESP_LOGW(MESH_TAG,"Eleicao acabou\n");
		break;
		case MESH_EVENT_LAYER_CHANGE: //The node's layer in the mesh network has changed
			ESP_LOGE(MESH_TAG,"MESH_EVENT_LAYER_CHANGE\n");
			mesh_layer = evento.info.layer_change.new_layer;
			last_layer = mesh_layer;
		break;
		case MESH_EVENT_CHANNEL_SWITCH: //The mesh wifi channel has changed
			ESP_LOGE(MESH_TAG,"MESH_EVENT_CHANNEL_SWITCH %d\n",evento.info.channel_switch.channel);
		break;
		//
		//MESH ROOT-SPECIFIC EVENTS
		//
		case MESH_EVENT_ROOT_GOT_IP: //The station DHCP client retrieves a dynamic IP configuration or a Static IP is applied
			ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_GOT_IP\n");
			ESP_LOGI(MESH_TAG,
                 	 "<MESH_EVENT_ROOT_GOT_IP>sta ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
                 	 IP2STR(&evento.info.got_ip.ip_info.ip),
                 	 IP2STR(&evento.info.got_ip.ip_info.netmask),
                 	 IP2STR(&evento.info.got_ip.ip_info.gw));
		
		break;
		case MESH_EVENT_ROOT_LOST_IP: //The lease time  of the Node's station dynamic IP configuration has expired
			ESP_LOGE(MESH_TAG,"MESH_EVENT_ROOT_LOST_IP\n");
		break;
		case MESH_EVENT_ROOT_SWITCH_REQ: //The root node has received a root switch request from  a candidate root
			ESP_LOGE(MESH_TAG,"MESH_EVENT_ROOT_SWITCH_REQ\n");
			printf(MAC2STR( evento.info.switch_req.rc_addr.addr));
		break;
		case MESH_EVENT_ROOT_ASKED_YIELD: //Another root node with a higher RSSI with the router has asked this root node to yield
			printf("MESH_EVENT_ROOT_ASKED_YIELD\n");
		break;
		case MESH_EVENT_SCAN_DONE:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_SCAN_DONE");
		break;
		default:
			ESP_LOGW(MESH_TAG,"MESH_EVENT NOT HANDLED");
		break;
	}

}

void meshf_init(){

	//Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //Inicializacao Wifi
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
}

void meshf_start(){
	/* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());
}