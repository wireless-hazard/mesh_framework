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

void app_main(void) {
	meshf_init();
	meshf_start();
	meshf_mqtt_start();

	char strftime_buff[64];
	char mqtt_string[256];

	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();

	time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI("TESTE", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    setenv("TZ", "UTC+3", 1);
	tzset();
    localtime_r(&now, &timeinfo);

	strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
	ESP_LOGI("TESTE", "The current date/time in UNIPAMPA is: %s", strftime_buff);

	// Hall effect sensor test
	gpio_pad_select_gpio(2);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(2, GPIO_MODE_OUTPUT);
	adc1_config_width(ADC_WIDTH_BIT_12);
	int lobby;
	while(true){
		lobby = hall_sensor_read();
		printf("HALL EFFECT VALUE = %d\n", lobby);
		if ((lobby <= 0) || (lobby >= 50)){
			gpio_set_level(2,1);

			time_t now = 0;
    		struct tm timeinfo = { 0 };
    		int retry = 0;
    		const int retry_count = 10;
    		time(&now);
    		setenv("TZ", "UTC+3", 1);
			tzset();
    		localtime_r(&now, &timeinfo);

			strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo);
			ESP_LOGI("TESTE", "The current date/time in UNIPAMPA is: %s", strftime_buff);
			sprintf(mqtt_string,"The current date/time in UNIPAMPA is: %s", strftime_buff);
			esp_mqtt_client_publish(mqtt_handler,"/sntp/esp32time",mqtt_string,0,0,0);

			meshf_sleep_time(2000);
			gpio_set_level(2,0);
		}
		meshf_sleep_time(1000);
	}

	
}