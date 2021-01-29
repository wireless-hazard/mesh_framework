#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
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
#include "esp_sleep.h"
#include <cJSON.h>

void app_main(void) {
	gpio_reset_pin(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    gpio_set_level(2, 1);

    char mqtt_data[150];
    char strftime_buff[64];
	time_t now = 0;
    struct tm timeinfo = { 0 }; 
    uint8_t self_mac[6] = {0,};

	uint8_t rx_mensagem[180] = {0,};
	int resp = 0;
    esp_sleep_wakeup_cause_t wakeUpCause; 

    meshf_init();
    wakeUpCause = esp_sleep_get_wakeup_cause();
	if (wakeUpCause == ESP_SLEEP_WAKEUP_TIMER){
		meshf_start();
		meshf_rx(rx_mensagem);
		
    }else{
		meshf_start();
		meshf_start_sntp();
		meshf_rx(rx_mensagem);
		meshf_asktime();
		time(&now);
		localtime_r(&now, &timeinfo);
		strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
		sprintf(mqtt_data, "The current date/time is: %s (year %d)",strftime_buff,timeinfo.tm_year);
		printf("%s\n",mqtt_data);

		int minutes = timeinfo.tm_min;
		if (minutes < 5){
			minutes = 5;
		}
		int rouded_minutes = (minutes / 5) * 5;
		int next_minutes = rouded_minutes + 5;
		if (next_minutes >=60){
			next_minutes -= 59;
		}
		printf("VAI ESPERAR ATE CHEGAR EM X HORAS E %d MINUTOS\n",next_minutes);
		meshf_sleep_time((next_minutes - minutes)*60000);
	}

	time(&now);
	setenv("TZ", "UTC+3", 1);
	tzset();
	localtime_r(&now, &timeinfo);

	meshf_start_mqtt();

	esp_read_mac(self_mac,ESP_MAC_WIFI_SOFTAP);
	strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
	sprintf(mqtt_data, "The current date/time in %02x:%02x:%02x:%02x:%02x:%02x is: %s", self_mac[0],self_mac[1],self_mac[2],self_mac[3],self_mac[4],self_mac[5],strftime_buff);
	resp = meshf_mqtt_publish("/data/esp32",strlen("/data/esp32"),mqtt_data,strlen(mqtt_data));
	printf("%s\n",esp_err_to_name(resp));

    meshf_sleep_time(60000);
	// esp_mesh_stop();
	time(&now);
	localtime_r(&now, &timeinfo);
	int minutesS = timeinfo.tm_min;
	if (minutesS < 5){
		minutesS = 5;
	}
	int rouded_minutesS = (minutesS / 5) * 5;
	int next_minutesS = rouded_minutesS + 5;
	if (next_minutesS >=60){
		next_minutesS -= 59;
	}
	printf("VAI ACORDAR QUANDO FOR X HORAS E %d MINUTOS\n",next_minutesS);
	esp_wifi_stop();
	esp_deep_sleep((next_minutesS - minutesS)*60000000);
	// char mac_destination[] = "80:7D:3A:B7:C8:18";
	// meshf_sleep_time(1000);
	// meshf_ping(mac_destination);
}