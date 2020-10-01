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
#include <esp_system.h>
#include <time.h>
#include <sys/time.h>
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"
#include "mesh_framework.h"

#include <driver/adc.h>
#include "driver/gpio.h"
#include "esp_sntp.h"

#include <cJSON.h>

void app_main(void) {
	meshf_init();
	meshf_start();
	meshf_mqtt_start();

	uint8_t topico[] = "/esp32/car_check";
	uint8_t data[] = "Um veiculo foi localizado";
	uint8_t teste[100] = {0,};

	meshf_rx(&teste);
	meshf_asktime();
	meshf_mqtt_publish(topico,data,sizeof(topico),sizeof(data));
	// char mqtt_string[256];
	// char strftime_buff[64];
	// // Hall effect sensor test
	// gpio_pad_select_gpio(2);
	// /* Set the GPIO as a push/pull output */
	// gpio_set_direction(2, GPIO_MODE_OUTPUT);
	// adc1_config_width(ADC_WIDTH_BIT_12);
	// int lobby;

	// meshf_asktime();
	// // while(true){
	// 	meshf_sleep_time(5000);
	// 	time_t now = 0;
	// 	time(&now);
 //    	struct tm timeinfo = { 0 };
	// 	localtime_r(&now, &timeinfo);
	// 	strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
	// 	ESP_LOGI("MESH_TAG", "UNIPAMPA is: %s", strftime_buff);

	// uint8_t tx_buffer[1460] = { 0, };
	// static mesh_data_t tx_data;
	// tx_data.data = tx_buffer;
	// uint8_t transmitted_data[] = "/DATA/SENTS";
	// tx_data.size = sizeof(transmitted_data);
	// memcpy(tx_data.data, transmitted_data, sizeof(transmitted_data));
	// int flag = MESH_DATA_P2P;
	// tx_data.proto = MESH_PROTO_BIN;
	// esp_err_t send_error = esp_mesh_send(NULL,&tx_data,flag,NULL,0);
	// ESP_LOGE("MESH_","Erro :%s na comunicacao p2p\n",esp_err_to_name(send_error));
	// data_ready();
	// for(int i=0;i<sizeof(teste);i++){
	// 	printf("%d %c\n",teste[i],(char)teste[i]);
	// }

	// printf("\n\n\n");
	// char *topic = malloc(teste[0]-1);
	// memcpy(topic,teste+6,teste[0]-1);
	// topic[teste[0]-1] ='\0';
	// printf("->%s<-\n",topic);
	// printf("\n\n\n");

	// char *received_data = malloc(teste[1]);
	// memcpy(received_data,teste+teste[0]+6,teste[1]);
	// received_data[teste[1]-1] = '\0';
	// printf("->%s<-\n",received_data);

	// esp_mqtt_client_publish(mqtt_handler,topic,received_data,0,0,0);


	// while(true){
	// 	lobby = hall_sensor_read();
	// 	printf("HALL EFFECT VALUE = %d\n", lobby);
	// 	if ((lobby <= 0) || (lobby >= 50)){
	// 		gpio_set_level(2,1);

	// 		time_t now = 0;
 //    		struct tm timeinfo = { 0 };
    		
 //    		time(&now);
 //    		localtime_r(&now, &timeinfo);

	// 		strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
	// 		ESP_LOGI("TESTE", "The current date/time in UNIPAMPA is: %s", strftime_buff);
	// 		sprintf(mqtt_string,"The current date/time in UNIPAMPA is: %s", strftime_buff);
	// 		if (esp_mesh_is_root()){
	// 			esp_mqtt_client_publish(mqtt_handler,"/sntp/esp32time",mqtt_string,0,0,0);
	// 		}
	// 		meshf_sleep_time(2000);
	// 		gpio_set_level(2,0);
	// 	}
	// 	meshf_sleep_time(1000);
	// }

	
}