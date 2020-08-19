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

void app_main(void) {
	char mac[] = "A4:CF:12:75:21:31"; //softAP
	meshf_init();
	meshf_start();
	meshf_mqtt_start();

	int msg_id = esp_mqtt_client_publish(mqtt_handler, "/teste/ando", "data_3", 0, 1, 0);
    printf("sent publish successful, msg_id=%d\n", msg_id);
    msg_id = esp_mqtt_client_subscribe(mqtt_handler, "/teste/send", 0);
    printf("sent subscribe successful, msg_id=%d\n", msg_id);
}