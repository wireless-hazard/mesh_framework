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

void time_message_generator(char final_message[]){ //Formata a data atual do ESP para dentro do array especificado
	char strftime_buff[64];
	time_t now = 0;
    struct tm timeinfo = { 0 }; 
    uint8_t self_mac[6] = {0,};

    time(&now); //Pega o tempo armazenado no RTC
	setenv("TZ", "UTC+3", 1); //Configura variaveis de ambiente com esse time zona
	tzset(); //Define a time zone
	localtime_r(&now, &timeinfo); //Reformata o horario pego do RTC

	esp_read_mac(self_mac,ESP_MAC_WIFI_SOFTAP); //Pega o MAC da interface Access Point
	strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
	sprintf(final_message, "The current date/time in %02x:%02x:%02x:%02x:%02x:%02x is: %s", self_mac[0],self_mac[1],self_mac[2],self_mac[3],self_mac[4],self_mac[5],strftime_buff);
}

int next_sleep_time(int fixed_gap){ //Recebe o valor em minutos e calcula em quantos segundos o ESP devera acordar, considerando o inicio em uma hora exata.
	if (fixed_gap <= 0){
		return 0;
	}

	time_t now = 0;
    struct tm timeinfo = { 0 };

    time(&now); //Pega o tempo armazenado no RTC
	setenv("TZ", "UTC+3", 1); //Configura variaveis de ambiente com esse time zona
	tzset(); //Define a time zone
	localtime_r(&now, &timeinfo); //Reformata o horario pego do RTC

	int minutes = timeinfo.tm_min;
	if (minutes < fixed_gap){
		minutes = fixed_gap;
	}
	int rouded_minutes = (minutes / fixed_gap) * fixed_gap;
	int next_minutes = rouded_minutes + fixed_gap - 1;
	int next_seconds = 60 - timeinfo.tm_sec;
	if (next_minutes >=59){
		next_minutes -= 58;
		return (next_minutes*60 + next_seconds);
	}
	return ((next_minutes - minutes)*60 + next_seconds);
}

void app_main(void) {

	gpio_reset_pin(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    gpio_set_level(2, 1);

    char mqtt_data[150];

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
		time_message_generator(mqtt_data);
		printf("%s\n",mqtt_data);

		int awake_until = next_sleep_time(5);

		printf("Vai continuar acordado por %d segundos\n",awake_until);
		meshf_sleep_time(awake_until*1000);
	}

	meshf_start_mqtt();

	time_message_generator(mqtt_data);
	resp = meshf_mqtt_publish("/data/esp32",strlen("/data/esp32"),mqtt_data,strlen(mqtt_data));
	printf("%s\n",esp_err_to_name(resp));

	meshf_sleep_time(60000);
	// esp_mesh_stop();

	int next_wakeup = next_sleep_time(5);

	printf("Ira acordar daqui a %d segundos\n",next_wakeup);
	esp_wifi_stop();
	esp_deep_sleep(next_wakeup*1000000);
	// char mac_destination[] = "80:7D:3A:B7:C8:18";
	// meshf_sleep_time(1000);
	// meshf_ping(mac_destination);
}