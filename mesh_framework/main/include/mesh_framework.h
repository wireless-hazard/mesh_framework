#include "mqtt_client.h"

void STR2MAC(uint8_t *address,char rec_string[17]);
uint16_t meshf_uint8_t_json_creator(uint8_t json_value[], uint8_t key[], uint16_t size_key, uint8_t value[], uint16_t size_value);
void tx_p2p(void *pvParameters);
void meshf_tx_p2p(char mac_destination[],uint8_t transmitted_data[],uint16_t data_size);
void meshf_tx_TODS(char ip_destination[],int port,uint8_t transmitted_data[],uint16_t data_size);
void meshf_rx(uint8_t *array_data);
void meshf_asktime();
int meshf_mqtt_publish(char topic[], uint16_t topic_size, char data[], uint16_t data_size);
void meshf_ping(char mac_destination[]);
void meshf_rssi_info(int8_t *rssi,char interested_mac[]);
void meshf_task_debugger(void);
void data_ready();
void free_rx_buffer();
void meshf_sleep_time(float delay);
void meshf_init();
void meshf_start();
void meshf_start_mqtt();
void meshf_start_sntp();

extern esp_mqtt_client_handle_t mqtt_handler;
extern mesh_data_t rx_data;
