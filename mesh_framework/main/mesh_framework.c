#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_mesh.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "lwip/sockets.h"
#include "mesh_framework.h"

#define MAX_LAYERS CONFIG_MAX_LAYERS
#define WIFI_CHANNEL 0
#define ROUTER_SSID CONFIG_ROUTER_SSID
#define ROUTER_PASSWORD CONFIG_ROUTER_PASSWORD
#define MAX_CLIENTS CONFIG_MAX_CLIENTS

#ifndef CONFIG_IS_TODS_ALLOWED
	#define CONFIG_IS_TODS_ALLOWED false
#endif

static SemaphoreHandle_t SemaphoreParentConnected = NULL;
static SemaphoreHandle_t SemaphoreDataReady = NULL;
 
static const uint8_t MESH_ID[6] = {0x05, 0x02, 0x96, 0x05, 0x02, 0x96};
static const char *MESH_TAG = "mesh_tagger";
static int mesh_layer = -1;
static mesh_addr_t mesh_parent_addr;

static bool is_mesh_connected = false;
static bool is_parent_connected = false;
static bool is_buffer_free = true;

static uint8_t tx_buffer[1460] = { 0, };
static uint8_t rx_buffer[MESH_MTU] = { 0, };

static mesh_addr_t tx_destination;
static mesh_data_t tx_data;

static mesh_addr_t rx_sender;
static mesh_data_t rx_data;

static mesh_addr_t toDS_destination;
static mesh_data_t toDS_data;

static int8_t *rssi_g = NULL;
static uint8_t mac[6] = {0,};
static wifi_scan_config_t cfg_scan = {
		.ssid = 0,
		.bssid = 0,
		.channel = 0,
		.show_hidden = 0,
		.scan_type = WIFI_SCAN_TYPE_ACTIVE,
		.scan_time.passive = 500,
		.scan_time.active.min = 120,
		.scan_time.active.max = 500,
};

void STR2MAC(uint8_t *address,char rec_string[17]){
	uint8_t mac[6] = {0,};
	int j = 0;
	for (int i = 0;i<=17;i+=3){
		if((uint8_t)rec_string[i] <= 58){
			mac[j] = (((uint8_t)rec_string[i]-48) * 16);
		}else{
			mac[j] = (((uint8_t)rec_string[i]-55) * 16);
		}
		if((uint8_t)rec_string[i+1] <= 58){
			mac[j] += ((uint8_t)rec_string[i+1]-48);	
		}else{
			mac[j] += ((uint8_t)rec_string[i+1]-55);
		}
		j++;
	}
	memcpy(address,&mac,sizeof(uint8_t)*6);
}

void tx_p2p(void *pvParameters){
	
	int flag = MESH_DATA_P2P;
	tx_data.proto = MESH_PROTO_BIN;
	uint8_t self_mac[6] = {0,};

	esp_wifi_get_mac(ESP_IF_WIFI_AP,&self_mac);

	if(self_mac[0]==tx_destination.addr[0] && self_mac[1]==tx_destination.addr[1] && self_mac[2]==tx_destination.addr[2] 
	&& self_mac[3]==tx_destination.addr[3] && self_mac[4]==tx_destination.addr[4] && self_mac[5]==tx_destination.addr[5]){
		ESP_LOGE(MESH_TAG,"NOT SENDING TO LOOPBACK INTERFACE");
		vTaskDelete(NULL);
	}

	esp_err_t send_error = esp_mesh_send(&tx_destination,&tx_data,flag,NULL,0);
	while (send_error != ESP_OK){
		ESP_LOGE(MESH_TAG,"Erro :%s na comunicacao p2p\n",esp_err_to_name(send_error));
		vTaskDelay(1*1000/portTICK_PERIOD_MS);
		send_error = esp_mesh_send(&tx_destination,&tx_data,flag,NULL,0);
	}
	ESP_LOGI(MESH_TAG,"DADOS ENVIADOS");
	vTaskDelete(NULL);
}

void send_external_net(void *pvParameters){
	
	mesh_addr_t from;
	mesh_addr_t to;
	mesh_data_t data;
	data.size = MESH_MTU;
	data.data = rx_buffer;
	mesh_rx_pending_t rx_pending;

	ip4_addr_t ipteste[4] = {0,};

	int flag = 0;

	while(true){
		ESP_ERROR_CHECK(esp_mesh_get_rx_pending(&rx_pending));
		// ESP_LOGI(MESH_TAG,"Numero de pacotes para rede externa: %d",rx_pending.toDS);
		if(rx_pending.toDS <= 0){
			vTaskDelay(10/portTICK_PERIOD_MS);
		}else{

			esp_err_t err = esp_mesh_recv_toDS(&from,&to,&data,0,&flag,NULL,0);
						
			ESP_LOGW(MESH_TAG, "vindos de: "MACSTR" dados: %d, size: %d",MAC2STR(from.addr),  (int8_t)data.data[1], data.size);
			ESP_LOGW(MESH_TAG,"Passando os pacotes via SOCKETS para o ip "IPSTR"",IP2STR(&to.mip.ip4));

			memcpy(ipteste,&to.mip.ip4.addr,sizeof(to.mip.ip4.addr));
			
			char rx_buffer[128];
    		char addr_str[128];
    		int addr_family;
    		int ip_protocol;
    		
			struct sockaddr_in destAddr;

			int Byte1 = ((int)ip4_addr1(ipteste));
    		int Byte2 = ((int)ip4_addr2(ipteste));
    		int Byte3 = ((int)ip4_addr3(ipteste));
    		int Byte4 = ((int)ip4_addr4(ipteste));

    		int data_size = data.size;
    		char ip_final[19]={0,};
    		char header[45];
    		char dados[((data_size-7)*3 + (data_size-7))];
    		char dados_final[45 + ((data_size-7)*3 + (data_size-7))];
    		
    		sprintf(ip_final,"%d.%d.%d.%d",Byte1,Byte2,Byte3,Byte4);

    		for (int i = 8; i < data_size; ++i){
    			if (i == 8){
					sprintf(dados,"%.3d;",data.data[i]);
				}else if (i == data_size - 1){
					sprintf(dados + (3*(i-8)+(i-8)),"%.3d",data.data[i]);
				}else{
					sprintf(dados + (3*(i-8)+(i-8)),"%.3d;",data.data[i]);
				}
			}
    		
    		sprintf(header,"%02x:%02x:%02x:%02x:%02x:%02x;%02x:%02x:%02x:%02x:%02x:%02x;%.3d;%.2d;",
    			from.addr[0],from.addr[1],from.addr[2],from.addr[3],from.addr[4],from.addr[5]+1,
    			data.data[7],data.data[6],data.data[5],data.data[4],data.data[3],data.data[2],
    			(int8_t)data.data[1],(int)data.data[0]);

    		sprintf(dados_final,"%s",header);
    		sprintf(dados_final + (int)sizeof(header) - 1,"%s",dados);
    		
    		ESP_LOGI(MESH_TAG,"%s",dados_final);

    		destAddr.sin_addr.s_addr = inet_addr(ip_final);
    		destAddr.sin_family = AF_INET;
    		destAddr.sin_port = htons((unsigned short)to.mip.port);
    		addr_family = AF_INET;
    		ip_protocol = IPPROTO_IP;
    		inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
	
			int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
			int error = connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
			if(error!=0){
				ESP_LOGE(MESH_TAG,"SERVIDOR SOCKET ESTA OFFLINE \n REINICIANDO CONEXAO");
				close(sock);
				xTaskCreatePinnedToCore(&send_external_net,"Recepcao",4096,NULL,5,NULL,1);
				vTaskDelete(NULL);	
			}	
			printf("Estado da conexao: %dK\n",error);
			send(sock,&dados_final,strlen(dados_final),0);
			close(sock);
		}	
	}
}

void meshf_tx_p2p(char mac_destination[], uint8_t transmitted_data[], uint16_t data_size){
	tx_data.data = tx_buffer;
	tx_data.size = data_size;

	memcpy(tx_data.data, transmitted_data, data_size);
	STR2MAC(tx_destination.addr,mac_destination);
	xTaskCreatePinnedToCore(&tx_p2p,"P2P transmission",4096,NULL,5,NULL,0);
}

void tx_TODS(void *pvParameters){
	uint8_t prov_buffer[8] = {0,};
	mesh_addr_t parent_bssid;
	
	int flag = MESH_DATA_TODS;
	wifi_ap_record_t apdata;

	esp_err_t err = esp_wifi_sta_get_ap_info(&apdata);
	ESP_LOGW(MESH_TAG,"rssi: %d\n",apdata.rssi);
	esp_err_t bssid_error = esp_mesh_get_parent_bssid(&parent_bssid);

	if (bssid_error == ESP_OK){
		prov_buffer[0] = (uint8_t)esp_mesh_get_layer();
		prov_buffer[1] = (uint8_t)apdata.rssi;
	}

	ESP_LOGE(MESH_TAG,"%s\n",esp_err_to_name(bssid_error));

	int j = 0;

	for (int i = 5; i >= 0; --i){
		prov_buffer[j+2] = parent_bssid.addr[i];
		++j;
	}

	memcpy(toDS_data.data, &prov_buffer, sizeof(prov_buffer));
	esp_err_t send_error = esp_mesh_send(&toDS_destination,&toDS_data,flag,NULL,0);

	if (send_error != ESP_OK){
		ESP_LOGE(MESH_TAG,"%s\n",esp_err_to_name(send_error));
		xTaskCreatePinnedToCore(&tx_TODS,"TO DS transmission",4096,NULL,5,NULL,0);
		vTaskDelete(NULL);
	}
	ESP_LOGW(MESH_TAG,"Sending the messagem to the external network");
	vTaskDelete(NULL);		
}

void meshf_tx_TODS(char ip_destination[],int port, uint8_t transmitted_data[], uint16_t data_size){
	
	toDS_data.data = tx_buffer;
	toDS_data.size = data_size + 8;
	toDS_data.proto = MESH_PROTO_BIN;

	toDS_destination.mip.port = port;
	toDS_destination.mip.ip4.addr = ipaddr_addr(ip_destination);

	memcpy(toDS_data.data + 8, transmitted_data, data_size);
	xTaskCreatePinnedToCore(&tx_TODS,"TO DS transmission",4096,NULL,5,NULL,0);
}

void rx_connection(void *pvParameters){

	uint8_t *array_data = (uint8_t *)pvParameters;
	rx_data.data = rx_buffer;
	rx_data.size = MESH_MTU;
	mesh_rx_pending_t rx_pending;
	int flag = 0;

	while(true){
		ESP_ERROR_CHECK(esp_mesh_get_rx_pending(&rx_pending));

		//ESP_LOGI(MESH_TAG,"Numero de pacotes para este ESP32: %d\n Estado do Semaforo: %d\n",rx_pending.toSelf,uxSemaphoreGetCount(SemaphoreDataReady));
		if(rx_pending.toSelf <= 0 || !is_buffer_free){
			vTaskDelay(10/portTICK_PERIOD_MS);
		}else{
			ESP_ERROR_CHECK(esp_mesh_recv(&rx_sender,&rx_data,0,&flag,NULL,0));
			memcpy(array_data,rx_data.data,rx_data.size);
			is_buffer_free = false;
			xSemaphoreGive(SemaphoreDataReady);
			ESP_LOGI(MESH_TAG,"DADOS RECEBIDOS");
		}
	}
} 

void meshf_rx(uint8_t *array_data){
	xTaskCreatePinnedToCore(&rx_connection,"P2P transmission",4096,((void *)array_data),5,NULL,1);
}

void rssi_info(void *pvParameters){
	//Disable self organizaned networking
	esp_mesh_set_self_organized(0,0);
	//Stop any scans already in progress
	esp_wifi_scan_stop();
	//Manually start scan. Will automatically stop when run to completion
	esp_wifi_scan_start(&cfg_scan,true);
	vTaskDelete(NULL);
}

void scan_complete(void *pvParameters){
	uint16_t phones = 0;
    wifi_ap_record_t *aps_list;
    esp_wifi_scan_get_ap_num(&phones);

    aps_list = (wifi_ap_record_t *)malloc(phones * sizeof(wifi_ap_record_t));

	esp_wifi_scan_get_ap_records(&phones, aps_list);
   	
    esp_mesh_set_self_organized(1,0);//Re-enable self organized networking if still connected
    for (int i = 0;i < phones;i++){
    	if (aps_list[i].bssid[0] == mac[0] && aps_list[i].bssid[1] == mac[1] && aps_list[i].bssid[2] == mac[2] && 
    		aps_list[i].bssid[3] == mac[3] && aps_list[i].bssid[4] == mac[4] &&  aps_list[i].bssid[5] == mac[5]){
    		ESP_LOGI(MESH_TAG,"SSID: %s    RSSI: %d    channel: %d",aps_list[i].ssid,aps_list[i].rssi, aps_list[i].primary);

    		*rssi_g = aps_list[i].rssi;
    		xSemaphoreGive(SemaphoreDataReady);

    		vTaskDelete(NULL);
    	}
    }
    ESP_LOGE(MESH_TAG,"AP Not found");
    *rssi_g = 0;
    xSemaphoreGive(SemaphoreDataReady);
    vTaskDelete(NULL);
}

void meshf_rssi_info(int8_t *rssi,char interested_mac[]){
	STR2MAC(mac,interested_mac);
	rssi_g = rssi;
	xTaskCreatePinnedToCore(&rssi_info,"RSSI info",4096,NULL,6,NULL,0);
}

void data_ready(){
	xSemaphoreTake(SemaphoreDataReady,portMAX_DELAY);
}

void free_rx_buffer(){
	is_buffer_free = true;
}

void meshf_sleep_time(float delay){
	vTaskDelay(delay/portTICK_PERIOD_MS);
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
			ESP_LOGW(MESH_TAG,"MESH_EVENT_STOPPED\n");
		break;
		case MESH_EVENT_PARENT_CONNECTED:
			//COPY AND PAST'ED 
			mesh_layer = evento.info.connected.self_layer;
			memcpy(&mesh_parent_addr.addr, evento.info.connected.connected.bssid, 6);
			last_layer = mesh_layer;
        	is_mesh_connected = true;
        	is_parent_connected = true;
        	xSemaphoreGive(SemaphoreParentConnected);
			if (esp_mesh_is_root()) {
            	tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
            	if (CONFIG_IS_TODS_ALLOWED){
            		xTaskCreatePinnedToCore(&send_external_net,"TO DS COMMUNICATION",4096,NULL,5,NULL,1);
            	}
        	}
    		ESP_LOGW(MESH_TAG,"MESH_EVENT_PARENT_CONNECTED\n");
    		
		break;
		case MESH_EVENT_PARENT_DISCONNECTED: //Perform a fixed number of attempts to reconnect before searching for another one
			is_mesh_connected = false;
			is_parent_connected = false;
			xSemaphoreTake(SemaphoreParentConnected,portMAX_DELAY);
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
			ESP_LOGW(MESH_TAG,"MESH_EVENT_LAYER_CHANGE\n");
			mesh_layer = evento.info.layer_change.new_layer;
			last_layer = mesh_layer;
		break;
		case MESH_EVENT_CHANNEL_SWITCH: //The mesh wifi channel has changed
			ESP_LOGW(MESH_TAG,"MESH_EVENT_CHANNEL_SWITCH %d\n",evento.info.channel_switch.channel);
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
			ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_LOST_IP\n");
		break;
		case MESH_EVENT_ROOT_SWITCH_REQ: //The root node has received a root switch request from  a candidate root
			ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_SWITCH_REQ\n");
			printf(MAC2STR( evento.info.switch_req.rc_addr.addr));
		break;
		case MESH_EVENT_ROOT_ASKED_YIELD: //Another root node with a higher RSSI with the router has asked this root node to yield
			printf("MESH_EVENT_ROOT_ASKED_YIELD\n");
		break;
		case MESH_EVENT_SCAN_DONE:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_SCAN_DONE");
			xTaskCreatePinnedToCore(&scan_complete,"SCAN DONE",4096,NULL,6,NULL,1);
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
    SemaphoreParentConnected = xSemaphoreCreateBinary();
	if (SemaphoreParentConnected == NULL){
		ESP_LOGE(MESH_TAG,"ERROR CREATING SEMAPHORE: PARENTCONNECTED");
	}
	SemaphoreDataReady = xSemaphoreCreateBinary();
	if (SemaphoreDataReady == NULL){
		ESP_LOGE(MESH_TAG,"ERROR CREATING SEMAPHORE: DATAREADY");
	}
}

void meshf_start(){
	/* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());
    /*Blocks the code's flow until the ESP connects to a parent*/
    ESP_LOGE(MESH_TAG,"NOT CONNECTED TO A PARENT YET");
    xSemaphoreTake(SemaphoreParentConnected,portMAX_DELAY);
    xSemaphoreGive(SemaphoreParentConnected);
    ESP_LOGI(MESH_TAG,"PARENT CONNECTED");
}