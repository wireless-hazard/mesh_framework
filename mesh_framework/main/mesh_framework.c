#include "mesh_framework.h"

#define MAX_LAYERS CONFIG_MAX_LAYERS
#define ROUTER_SSID CONFIG_ROUTER_SSID
#define ROUTER_PASSWORD CONFIG_ROUTER_PASSWORD
#define MAX_CLIENTS CONFIG_MAX_CLIENTS
#define ROUTER_CHANNEL CONFIG_ROUTER_CHANNEL

#ifndef CONFIG_IS_TODS_ALLOWED
	#define CONFIG_IS_TODS_ALLOWED false
#endif

RTC_DATA_ATTR bool sntp_up2date;

static SemaphoreHandle_t SemaphoreParentConnected = NULL;
static SemaphoreHandle_t SemaphoreBrokerConnected = NULL;
static SemaphoreHandle_t SemaphoreDataReady = NULL;
static SemaphoreHandle_t SemaphoreSNTPConnected = NULL;
static SemaphoreHandle_t SemaphorePONG = NULL;
static SemaphoreHandle_t SemaphoreSNTPNODE = NULL;

static const uint8_t MESH_ID[6] = {0x05, 0x02, 0x96, 0x05, 0x02, 0x96};
static const char *MESH_TAG = "mesh_tagger";
static int mesh_layer = -1;
static mesh_addr_t mesh_parent_addr;

static esp_netif_t *netif_sta = NULL;

static bool is_mesh_connected = false;
static bool is_parent_connected = false;
static bool is_buffer_free = true;

static uint8_t tx_buffer[1460] = { 0, };
static uint8_t rx_buffer[MESH_MTU] = { 0, };

static mesh_addr_t tx_destination;
static mesh_data_t tx_data;

static mesh_addr_t rx_sender;
mesh_data_t rx_data;

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

esp_mqtt_client_handle_t mqtt_handler;

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

uint16_t meshf_uint8_t_json_creator(uint8_t json_value[], uint8_t key[], uint16_t size_key, uint8_t value[], uint16_t size_value){
	uint8_t front_header[] =  "{ \"";
	uint8_t inter_header[] = "\" :\"";
	uint8_t back_header[] = "\"}";
	int16_t index = 0;

	memcpy(json_value+index,&front_header,sizeof(front_header));
	index = index + sizeof(front_header)-1;
	memcpy(json_value+index,key,size_key);
	index = index + size_key-1;
	memcpy(json_value+index,&inter_header,sizeof(inter_header));
	index = index + sizeof(inter_header) - 1;
	memcpy(json_value+index,value,size_value);
	index = index + size_value - 1;
	memcpy(json_value+index,&back_header,sizeof(back_header));
	index = index + sizeof(back_header) - 1;
	return index;
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
    		
   //  		sprintf(header,"%02x:%02x:%02x:%02x:%02x:%02x;%02x:%02x:%02x:%02x:%02x:%02x;%.3d;%.2d;",
   //  			from.addr[0],from.addr[1],from.addr[2],from.addr[3],from.addr[4],from.addr[5]+1,
   //  			data.data[7],data.data[6],data.data[5],data.data[4],data.data[3],data.data[2],
   //  			(int8_t)data.data[1],(int)data.data[0]);

   //  		sprintf(dados_final,"%s",header);
   //  		sprintf(dados_final + (int)sizeof(header) - 1,"%s",dados);
    		
   //  		ESP_LOGI(MESH_TAG,"%s",dados_final);

   //  		destAddr.sin_addr.s_addr = inet_addr(ip_final);
   //  		destAddr.sin_family = AF_INET;
   //  		destAddr.sin_port = htons((unsigned short)to.mip.port);
   //  		addr_family = AF_INET;
   //  		ip_protocol = IPPROTO_IP;
   //  		inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
	
			// int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
			// int error = connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
			// if(error!=0){
			// 	ESP_LOGE(MESH_TAG,"SERVIDOR SOCKET ESTA OFFLINE \n REINICIANDO CONEXAO");
			// 	close(sock);
			// 	xTaskCreatePinnedToCore(&send_external_net,"Recepcao",4096,NULL,5,NULL,1);
			// 	vTaskDelete(NULL);	
			// }	
			// printf("Estado da conexao: %dK\n",error);
			// send(sock,&dados_final,strlen(dados_final),0);
			// close(sock);
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
		rx_data.data = rx_buffer;
		rx_data.size = MESH_MTU;

		ESP_ERROR_CHECK(esp_mesh_get_rx_pending(&rx_pending));

		// ESP_LOGI(MESH_TAG,"Numero de pacotes para este ESP32: %d\n Estado do Semaforo: %d\n",rx_pending.toSelf,uxSemaphoreGetCount(SemaphoreDataReady));
		if(rx_pending.toSelf <= 0 || !is_buffer_free){
			vTaskDelay(10/portTICK_PERIOD_MS);
		}else{
			ESP_ERROR_CHECK(esp_mesh_recv(&rx_sender,&rx_data,0,&flag,NULL,0));
			char data_json[180] = {0,};
			
			sprintf(data_json,"%s", rx_data.data);
			cJSON *json = cJSON_Parse(data_json);
			char *string1 = cJSON_Print(json);
			printf("%s\n",string1);
			cJSON *json_flag_data = cJSON_GetObjectItemCaseSensitive(json, "flag");

			if (strcmp(json_flag_data->valuestring,"PING") == 0){
				uint8_t buffer[20] = {0,};
				mesh_data_t pong_packet;
				pong_packet.data = buffer;
				pong_packet.size = 20;
				pong_packet.proto = MESH_PROTO_JSON;
				
				cJSON *json_pong = cJSON_CreateObject();
				cJSON_AddStringToObject(json_pong,"flag","PONG");
				char *string2 = cJSON_Print(json_pong);
				
				for(int i = 0; i<19;i++){
					pong_packet.data[i] = string2[i];
				}

				free(string2);

				printf("%s\n",pong_packet.data);
				esp_err_t send_error;
				if (esp_mesh_is_root()){
					send_error = esp_mesh_send(&rx_sender,&pong_packet,MESH_DATA_FROMDS,NULL,0);
				}else{
					send_error = esp_mesh_send(&rx_sender,&pong_packet,flag,NULL,0);
				}
				ESP_LOGW("MESH_TAG","Sending PING RESPONSE = %s\n",esp_err_to_name(send_error));	
				continue;
			}else if (strcmp(json_flag_data->valuestring,"PONG") == 0){
				xSemaphoreGive(SemaphorePONG);
				continue;	
			}else if (strcmp(json_flag_data->valuestring,"SNTP") == 0){
				ESP_LOGE(MESH_TAG,"REQUEST SNTP");

				char strftime_buff[64];
				time_t now = 0;
    			struct tm timeinfo = { 0 };
    		
    			if (!sntp_up2date){
    				xSemaphoreTake(SemaphoreSNTPConnected,portMAX_DELAY);
    				xSemaphoreGive(SemaphoreSNTPConnected);	
    			}

    			time(&now);
    			localtime_r(&now, &timeinfo);

				strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
				ESP_LOGI(MESH_TAG, "The current date/time in UNIPAMPA is: %s", strftime_buff);

				cJSON *json_ptns = cJSON_CreateObject();
				cJSON_AddStringToObject(json_ptns,"flag","PTNS");
				cJSON_AddNumberToObject(json_ptns,"time",now);
				char *string = cJSON_Print(json_ptns);
				printf("%s\n",string);

				uint8_t buffer[41] = {0,};

				mesh_data_t sntp_packet;
				sntp_packet.data = buffer;
				sntp_packet.size = 41;
				sntp_packet.proto = MESH_PROTO_JSON;
				
				for(int i = 0; i < 40; i++){
					sntp_packet.data[i] = string[i];
				}
				printf("%s\n",sntp_packet.data);
				free(string);
				cJSON_Delete(json_ptns);
				esp_err_t send_error = esp_mesh_send(&rx_sender,&sntp_packet,MESH_DATA_FROMDS,NULL,0);
				ESP_LOGW("MESH_TAG","Sending SNTP RESPONSE = %s\n",esp_err_to_name(send_error));
				continue;
			}else if (strcmp(json_flag_data->valuestring,"PTNS") == 0){
				
				cJSON *json_time_data = cJSON_GetObjectItemCaseSensitive(json,"time");

				struct timeval tv;//Cria a estrutura temporaria para funcao abaixo.
  				tv.tv_sec = json_time_data->valueint;//Atribui minha data atual. Voce pode usar o NTP para isso ou o site citado no artigo!
  				settimeofday(&tv, NULL);

				char strftime_buff[64];
				time_t now = 0;
				time(&now);
				setenv("TZ", "UTC+3", 1);
				tzset();
    			struct tm timeinfo = { 0 };

    			localtime_r(&now, &timeinfo);

				strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
				ESP_LOGI(MESH_TAG, "The current date/time RECEIVED in UNIPAMPA is: %s", strftime_buff);

				ESP_LOGE(MESH_TAG,"RESPONSE SNTP");
				xSemaphoreGive(SemaphoreSNTPNODE);
				continue;

			}else if(strcmp(json_flag_data->valuestring,"MQTT") == 0){
				ESP_LOGW(MESH_TAG,"MQTT PUBLISH REQUEST");
				cJSON *json_topic = cJSON_GetObjectItemCaseSensitive(json,"topic");
				ESP_LOGI(MESH_TAG,"Topic ->%s<-\n",json_topic->valuestring);
				cJSON *json_published = cJSON_GetObjectItemCaseSensitive(json,"data");
				ESP_LOGI(MESH_TAG,"Data ->%s<-\n",json_published->valuestring);
				xSemaphoreTake(SemaphoreBrokerConnected,portMAX_DELAY);
				xSemaphoreGive(SemaphoreBrokerConnected);
				esp_mqtt_client_publish(mqtt_handler,json_topic->valuestring,json_published->valuestring,0,0,0);
				free(json_topic);
				free(json_published);
				continue;
			}
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

void task_asktime(void *pvParameters){
	uint8_t buffer[20] = {0,};
	cJSON *json_sntp = cJSON_CreateObject();
	cJSON_AddStringToObject(json_sntp,"flag","SNTP");
	char *string = cJSON_Print(json_sntp);
	mesh_data_t sntp_packet;
	sntp_packet.data = buffer;
	sntp_packet.size = 20;
	sntp_packet.proto = MESH_PROTO_JSON;
	sntp_packet.tos = MESH_TOS_P2P;
	for (int i = 0; i<19; i++){
		sntp_packet.data[i] = string[i];
	}
	esp_err_t send_error = esp_mesh_send(NULL,&sntp_packet,MESH_DATA_P2P,NULL,0);
	ESP_LOGW("MESH_TAG","Sending SNTP REQUEST = %s\n",esp_err_to_name(send_error));
	free(string);
	cJSON_Delete(json_sntp);
	vTaskDelete(NULL);
	
}

void meshf_asktime(){
	if (!esp_mesh_is_root()){ //THIS LINE ADDS AN WEIRD BUG
		xTaskCreatePinnedToCore(&task_asktime,"NODE ASK SNTP",4096,NULL,5,NULL,1);
		xSemaphoreTake(SemaphoreSNTPNODE,portMAX_DELAY);
		xSemaphoreGive(SemaphoreSNTPNODE);
		sntp_up2date = true;
	}else{
		if (!sntp_up2date){
			xSemaphoreTake(SemaphoreSNTPConnected,portMAX_DELAY);
			xSemaphoreGive(SemaphoreSNTPConnected);
		}
	}
}

int meshf_mqtt_publish(char topic[], uint16_t topic_size, char data[], uint16_t data_size){
	if (!esp_mesh_is_root()){
		cJSON *json_mqtt = cJSON_CreateObject();
		cJSON_AddStringToObject(json_mqtt,"flag","MQTT");
		cJSON_AddStringToObject(json_mqtt,"topic",topic);
		cJSON_AddStringToObject(json_mqtt,"data",data);
		char *string = cJSON_Print(json_mqtt);
		mesh_data_t tx_data;
		tx_data.data = tx_buffer;
		tx_data.size = data_size + topic_size + 48;
		tx_data.proto = MESH_PROTO_JSON;
		tx_data.tos = MESH_TOS_P2P;
		for(int i = 0; i < tx_data.size-1;i++){
			tx_data.data[i] = string[i];
		}
		printf("%s\n",tx_data.data);
		esp_err_t send_error = esp_mesh_send(NULL,&tx_data,MESH_DATA_P2P,NULL,0);
		ESP_LOGE(MESH_TAG,"Erro :%s na publicacao MQTT p2p\n",esp_err_to_name(send_error));
		cJSON_Delete(json_mqtt);
		free(string);
		return send_error;
	}else{
		ESP_LOGI(MESH_TAG,"Topic ->%s<-\n",topic);
		ESP_LOGI(MESH_TAG,"Data ->%s<-\n",data);
		xSemaphoreTake(SemaphoreBrokerConnected,portMAX_DELAY);
   		xSemaphoreGive(SemaphoreBrokerConnected);
		int send_error = esp_mqtt_client_publish(mqtt_handler,topic,data,0,0,0);
		if (send_error != 0){ //Zero indica que não houve um erro ao tentar publicar
			return ESP_FAIL;
		}
		return ESP_OK;
	}
}

void pinging(void *pvParameters){
	char *mac_destination = (char *)pvParameters;
	printf("%s\n",mac_destination);
	uint8_t buffer[20] = {0,};
	cJSON *json_ping = cJSON_CreateObject();
	cJSON_AddStringToObject(json_ping,"flag","PING");
	char *string1 = cJSON_Print(json_ping);
	
	mesh_addr_t ping_destination;
	STR2MAC(ping_destination.addr,mac_destination);
	mesh_data_t ping_packet;
	ping_packet.data = buffer;
	ping_packet.size = 20;
	ping_packet.proto = MESH_PROTO_JSON;
	
	for(int i = 0; i<19;i++){
		ping_packet.data[i] = string1[i];
	}

	free(string1);

	esp_err_t send_error = esp_mesh_send(&tx_destination,&ping_packet,MESH_DATA_P2P,NULL,0);
	if (send_error == ESP_OK){
		struct timeval b4;

		gettimeofday(&b4, NULL);

		unsigned long long b4inMS = (unsigned long long)(b4.tv_sec) * 1000 + (unsigned long long)(b4.tv_usec) / 1000;
		ESP_LOGW("MESH_TAG","Sending PING = %s\n",esp_err_to_name(send_error));
		xSemaphoreTake(SemaphorePONG,portMAX_DELAY);

		struct timeval after;

		gettimeofday(&after, NULL);

		unsigned long long afterinMS = (unsigned long long)(after.tv_sec) * 1000 + (unsigned long long)(after.tv_usec) / 1000;
		unsigned long long delay = afterinMS - b4inMS;
		ESP_LOGW("MESH_TAG","PONG Received\n after(%lld) - before(%lld) = delay: %lld ms",afterinMS,b4inMS,delay);
	}else{
		ESP_LOGW("MESH_TAG","Sending PING = %s\n",esp_err_to_name(send_error));
	}
	cJSON_Delete(json_ping);
	vTaskDelete(NULL);
}

void meshf_ping(char mac_destination[]){
	xTaskCreatePinnedToCore(&pinging,"Ping an ESP32",4096,((void *)mac_destination),5,NULL,1);	
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

void meshf_task_debugger(void){
	size_t free_memory = xPortGetFreeHeapSize();
	printf("\n\nFree heap memory: %d bytes\n\n",free_memory);
	char pcWriterBuffer[2400] = {0,};
	vTaskList(pcWriterBuffer);
	printf("%s",pcWriterBuffer);
	printf("\n\n");
	vTaskGetRunTimeStats(pcWriterBuffer);
	printf("%s",pcWriterBuffer);
	printf("\n\n");
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

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
	switch (event_id){
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
        	mesh_layer = esp_mesh_get_layer();
			ESP_LOGW(MESH_TAG,"MESH_EVENT_PARENT_DISCONNECTED\n");
		break;
		case MESH_EVENT_NO_PARENT_FOUND:
			ESP_LOGW(MESH_TAG,"eternamente IDLE\nESP32 ira reiniciar\n");
			//esp_restart();

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
			ESP_LOGW(MESH_TAG,"MESH_EVENT_CHILD_DISCONNECTED %d\n",filhos);
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
			ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_FIXED\n");
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
		break;
		case MESH_EVENT_CHANNEL_SWITCH: //The mesh wifi channel has changed
			ESP_LOGW(MESH_TAG,"MESH_EVENT_CHANNEL_SWITCH");
		break;
		//
		//MESH ROOT-SPECIFIC EVENTS
		//
		// case MESH_EVENT_ROOT_GOT_IP: //The station DHCP client retrieves a dynamic IP configuration or a Static IP is applied
		// 	ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_GOT_IP\n");
		// 	ESP_LOGI(MESH_TAG,
  //                	 "<MESH_EVENT_ROOT_GOT_IP>sta ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
  //                	 IP2STR(&evento.info.got_ip.ip_info.ip),
  //                	 IP2STR(&evento.info.got_ip.ip_info.netmask),
  //                	 IP2STR(&evento.info.got_ip.ip_info.gw));
		
		// break;
		// case MESH_EVENT_ROOT_LOST_IP: //The lease time  of the Node's station dynamic IP configuration has expired
		// 	ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_LOST_IP\n");
		// break;
		case MESH_EVENT_ROOT_SWITCH_REQ: //The root node has received a root switch request from  a candidate root
			ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_SWITCH_REQ\n");
		break;
		case MESH_EVENT_ROOT_SWITCH_ACK: //The root node has received a root switch request from  a candidate root
			ESP_LOGW(MESH_TAG,"MESH_EVENT_ROOT_SWITCH_ACK\n");
		break;
		case MESH_EVENT_ROOT_ASKED_YIELD: //Another root node with a higher RSSI with the router has asked this root node to yield
			printf("MESH_EVENT_ROOT_ASKED_YIELD\n");
		break;
		case MESH_EVENT_SCAN_DONE:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_SCAN_DONE");
			xTaskCreatePinnedToCore(&scan_complete,"SCAN DONE",4096,NULL,6,NULL,1);
		break;
		case MESH_EVENT_NETWORK_STATE:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_NETWORK_STATE");
		break;
		case MESH_EVENT_STOP_RECONNECTION:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_STOP_RECONNECTION");
		break;
		case MESH_EVENT_FIND_NETWORK:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_FIND_NETWORK");
		break;
		case MESH_EVENT_ROUTER_SWITCH:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_ROUTER_SWITCH");
		break;
		case  MESH_EVENT_PS_PARENT_DUTY:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_PS_PARENT_DUTY");
		break;
		case  MESH_EVENT_PS_CHILD_DUTY:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_PS_CHILD_DUTY");
		break;
		case MESH_EVENT_MAX:
			ESP_LOGW(MESH_TAG,"MESH_EVENT_MAX");
		break;
		default:
			ESP_LOGW(MESH_TAG,"MESH_EVENT NOT HANDLED");
		break;
	}

}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event){

	mqtt_handler = event->client;
	
	switch (event->event_id) {
		BaseType_t xStatus;
		
		case MQTT_EVENT_CONNECTED:
			ESP_LOGW(MESH_TAG,"MQTT_EVENT_CONNECTED");
			xSemaphoreGive(SemaphoreBrokerConnected);
		break;
		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGW(MESH_TAG,"MQTT_EVENT_DISCONNECTED");
		break;
		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGW(MESH_TAG,"MQTT_EVENT_SUBSCRIBED");
		break;
		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGW(MESH_TAG,"MQTT_EVENT_UNSUBSCRIBED");
		break;
		case MQTT_EVENT_PUBLISHED:
			ESP_LOGW(MESH_TAG,"MQTT_EVENT_PUBLISHED");
		break;
		case MQTT_EVENT_DATA:
			ESP_LOGW(MESH_TAG,"MQTT_EVENT_DATA");
			printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
		break;
		case MQTT_EVENT_ERROR:
			ESP_LOGW(MESH_TAG,"MQTT_EVENT_ERROR");
		break;
		case MQTT_EVENT_BEFORE_CONNECT:
			ESP_LOGW(MESH_TAG,"MQTT_EVENT_BEFORE_CONNECT");
		break;
		default:
            ESP_LOGI(MESH_TAG, "MQTT_EVENT NOT HANDLED, ID:%d", event->event_id);
        break;
	}
	return ESP_OK;
}

void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));

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
	ESP_ERROR_CHECK(esp_netif_init()); //Inicializa as estruturas de dados TCP/LwIP e cria a tarefa principal LwIP
		
	ESP_ERROR_CHECK(esp_event_loop_create_default()); //Lida com os eventos AINDA NAO IMPLEMENTADOS
	ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //
	esp_wifi_init(&cfg); //Inicia o Wifi com os seus parametros padroes
	ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
	// ESP_ERROR_CHECK(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA));//Desliga o cliente dhcp
	// ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));//Desliga o servidor dhcp
	
	ESP_ERROR_CHECK(esp_wifi_start());

	// Inicializacao Do MESH
	
	ESP_ERROR_CHECK(esp_mesh_init()); //Inicializa o "mesh stack"
	ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
	ESP_ERROR_CHECK(esp_mesh_set_topology(MESH_TOPO_TREE));
	ESP_ERROR_CHECK(esp_mesh_set_max_layer(MAX_LAYERS));//Numero maximo de niveis da rede
	ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(0.9));//Porcentagem minima para a escolha do Noh raiz
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(40));//Tempo sem comunicacao entre pai e filho com que fara a dissociacao do filho
	
	mesh_cfg_t config_mesh = MESH_INIT_CONFIG_DEFAULT(); //Possui a configuracao base mesh para aplicar depois
	
	//Mesh Network Identifier (MID)
	memcpy((uint8_t *) &config_mesh.mesh_id,MESH_ID,6);
	
	config_mesh.channel = ROUTER_CHANNEL;
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
	SemaphoreSNTPConnected = xSemaphoreCreateBinary();
	if (SemaphoreSNTPConnected == NULL){
		ESP_LOGE(MESH_TAG,"ERROR CREATING SEMAPHORE: SNTPCONNECTED");
	}
	SemaphoreSNTPNODE = xSemaphoreCreateBinary();
	if (SemaphoreSNTPNODE == NULL){
		ESP_LOGE(MESH_TAG,"ERROR CREATING SEMAPHORE: SNTPNODE");
	}
	SemaphorePONG = xSemaphoreCreateBinary();
	SemaphoreBrokerConnected = xSemaphoreCreateBinary();
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

void task_start_sntp(void *pvParameters){
	char strftime_buff[64];
		
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, CONFIG_SNTP_SERVER);
	
	sntp_init();

	time_t now = 0;
  	struct tm timeinfo = { 0 };
  	int retry = 0;
   	const int retry_count = 10;
   	while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
   	   	ESP_LOGI(MESH_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
       	vTaskDelay(2000 / portTICK_PERIOD_MS);
   	}
   	time(&now);
   	setenv("TZ", "UTC+3", 1);
	tzset();
   	localtime_r(&now, &timeinfo);

	strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
	ESP_LOGI(MESH_TAG, "The current date/time in UNIPAMPA is: %s", strftime_buff);
	xSemaphoreGive(SemaphoreSNTPConnected);
	sntp_up2date = true;
	vTaskDelete(NULL);
}

void meshf_start_sntp(){

	sntp_up2date = false;
	xSemaphoreTake(SemaphoreParentConnected,portMAX_DELAY);
    xSemaphoreGive(SemaphoreParentConnected);
	if (esp_mesh_is_root()){
    	xTaskCreatePinnedToCore(&task_start_sntp,"task_start_sntp",4096,NULL,5,NULL,0);		
	}
}

void task_start_mqtt(void *pvParameters){
	esp_mqtt_client_config_t mqtt_cfg = {
    	.uri = CONFIG_BROKER_URL,
    	.event_handle = mqtt_event_handler,
    };

   	xSemaphoreTake(SemaphoreParentConnected,portMAX_DELAY);
   	xSemaphoreGive(SemaphoreParentConnected);

   	esp_mqtt_client_handle_t mqtt_handler = esp_mqtt_client_init(&mqtt_cfg);
   	esp_mqtt_client_start(mqtt_handler);

   	xSemaphoreTake(SemaphoreBrokerConnected,portMAX_DELAY);
   	xSemaphoreGive(SemaphoreBrokerConnected);
   	vTaskDelete(NULL);
}

void meshf_start_mqtt(){
	xSemaphoreTake(SemaphoreParentConnected,portMAX_DELAY);
    xSemaphoreGive(SemaphoreParentConnected);
	if (esp_mesh_is_root()){
		xTaskCreatePinnedToCore(&task_start_mqtt,"task_start_mqtt",4096,NULL,5,NULL,0);		
	}
}