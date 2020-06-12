void tx_p2p(void *pvParameters);
void meshf_tx_p2p(char mac_destination[],uint8_t transmitted_data[],uint16_t data_size);
void mesh_event_handler(mesh_event_t evento);
void meshf_init();
void meshf_start();