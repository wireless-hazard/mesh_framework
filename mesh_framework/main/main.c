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
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"
#include "mesh_framework.h"

#include <driver/adc.h>
#include "driver/gpio.h"
#include "esp_sntp.h"

void meshf_asktime(){
	uint8_t buffer[1460] = {0,};
	uint8_t sntp_data[] = {'S','N','T','P'};
	mesh_data_t sntp_packet;
	sntp_packet.data = buffer;
	sntp_packet.size = 8*4;
	sntp_packet.proto = MESH_PROTO_BIN;
	memcpy(sntp_packet.data,&sntp_data,8*4);
	esp_err_t send_error = esp_mesh_send(NULL,&sntp_packet,MESH_DATA_P2P,NULL,0);
	ESP_LOGI("MESH_TAG","Sending SNTP REQUEST = %s\n",esp_err_to_name(send_error));

}

void app_main(void) {
	meshf_init();
	meshf_start();
	//meshf_mqtt_start();

	uint8_t teste[8] = {0,};

	meshf_rx(&teste);

	char mqtt_string[256];
	char strftime_buff[64];
	// Hall effect sensor test
	gpio_pad_select_gpio(2);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(2, GPIO_MODE_OUTPUT);
	adc1_config_width(ADC_WIDTH_BIT_12);
	int lobby;
	meshf_asktime();
	data_ready();
	ESP_LOGE("TAG","%d %d %d %d",teste[0],teste[1],teste[2],teste[3]);
	free_rx_buffer();
	data_ready();
	ESP_LOGE("TAG","%d %d %d %d",teste[0],teste[1],teste[2],teste[3]);
	free_rx_buffer();
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