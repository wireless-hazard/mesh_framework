#include "mqtt_client.h"

void STR2MAC(uint8_t *address,char rec_string[17]);
void tx_p2p(void *pvParameters);
void meshf_tx_p2p(char mac_destination[],uint8_t transmitted_data[],uint16_t data_size);
void meshf_tx_TODS(char ip_destination[],int port,uint8_t transmitted_data[],uint16_t data_size);
void meshf_rx(uint8_t *array_data);
void meshf_asktime();
void meshf_rssi_info(int8_t *rssi,char interested_mac[]);
void meshf_task_debugger(void);
void data_ready();
void free_rx_buffer();
void meshf_sleep_time(float delay);
void mesh_event_handler(mesh_event_t evento);
void meshf_mqtt_start();
void meshf_init();
void meshf_start();

extern esp_mqtt_client_handle_t mqtt_handler;
