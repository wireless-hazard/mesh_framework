#include <string.h>
#include <esp_system.h>
#include <time.h>
#include <sys/time.h>

// #include <ultrasonic.h>

#include "mesh_framework.h"

#include <driver/adc.h>
#include "driver/gpio.h"
#include "esp_sleep.h"

#define MAX_LAYERS CONFIG_MAX_LAYERS
#define ROUTER_CHANNEL CONFIG_ROUTER_CHANNEL
#define ROUTER_SSID CONFIG_ROUTER_SSID
#define ROUTER_PASSWORD CONFIG_ROUTER_PASSWORD
#define MAX_CLIENTS CONFIG_MAX_CLIENTS

#define MAX_DISTANCE_CM 500 // 5m max
#define TRIGGER_GPIO GPIO_NUM_23
#define ECHO_GPIO GPIO_NUM_22

RTC_DATA_ATTR float num_of_wakes[10];
static const uint8_t root_node[] = {0x80,0x7d,0x3a,0xb7,0xc8,0x19};
// static ultrasonic_sensor_t sensor;

void time_message_generator(char final_message[], int value){ //Formata a data atual do ESP para dentro do array especificado
	char strftime_buff[64];
	time_t now = 0;
    struct tm timeinfo = { 0 }; 
    uint8_t self_mac[6] = {0};
    char mac_str[18] = {0};
    char parent_mac_str[18] = {0};

    wifi_ap_record_t mesh_parent;

    time(&now); //Pega o tempo armazenado no RTC
	setenv("TZ", "UTC+3", 1); //Configura variaveis de ambiente com esse time zona
	tzset(); //Define a time zone
	localtime_r(&now, &timeinfo); //Reformata o horario pego do RTC

	esp_read_mac(self_mac,ESP_MAC_WIFI_SOFTAP); //Pega o MAC da interface Access Point
	strftime(strftime_buff, sizeof(strftime_buff), "%c", &timeinfo); //Passa a struct anterior para uma string
	sprintf(final_message, "%s",strftime_buff);
	sprintf(mac_str,MACSTR,MAC2STR(self_mac));

	esp_wifi_sta_get_ap_info(&mesh_parent);
	sprintf(parent_mac_str,MACSTR,MAC2STR(mesh_parent.bssid));



	cJSON *json_mqtt = cJSON_CreateObject();
	cJSON_AddStringToObject(json_mqtt,"mac", mac_str);
	cJSON_AddStringToObject(json_mqtt,"parent_mac", parent_mac_str);
	cJSON_AddNumberToObject(json_mqtt,"distance", value);
	cJSON_AddStringToObject(json_mqtt,"time", final_message);

	char *string2 = cJSON_Print(json_mqtt);
	strcpy(final_message,string2);
	free(string2);
	free(json_mqtt);
}

int next_sleep_time(const struct tm timeinfo, int fixed_gap){ //Recebe o valor em minutos e calcula em quantos segundos o ESP devera acordar, considerando o inicio em uma hora exata.
	if (fixed_gap <= 0){ //Se o valor do delay for de 0 minutos, retorna 0 minutos sem fazer nenhum calculo
		return 0;
	}

	int minutes = timeinfo.tm_min; //Pega o valor dos minutos atuais do RTC
	int rouded_minutes = (minutes / fixed_gap) * fixed_gap; //Transforma o valor dos minutos atuais do RTC o multiplo anterior de fixed_gap 
	int next_minutes = rouded_minutes + fixed_gap - 1; //Calcula qual o prox valor de minutos que seja multiplo de fixed_gap 
	int next_seconds = 60 - timeinfo.tm_sec; //Calcula o valor de segundos até que seja o minuto exato: XX:XX:00

	return ((next_minutes - minutes)*60 + next_seconds); //Retorna o tempo em segundos até que o ESP tenha seu RTC a XX:AA:00 sendo AA o prox valor em minutos multiplo de fixed_gap
}

esp_err_t measure_distance(float *distance){
	
    #if 0 
    esp_err_t err = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, distance);
    *distance = (*distance)*100.0;
    #endif
    esp_err_t err = ESP_OK;

    gpio_config_t io_conf = {
        .pin_bit_mask = GPIO_NUM_4,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    printf("GPIO_NUM_4 LEVEL: %d", gpio_get_level(GPIO_NUM_4));
    *distance = gpio_get_level(GPIO_NUM_4);

    return err; 
    // return ESP_OK;   
}

void use_network(char mqtt_data[], uint8_t rx_mensagem[]){
	meshf_start(); //Inicializa a rede MESH
	meshf_rx(rx_mensagem); //Seta o buffer para recepcao das mensagens
	meshf_start_mqtt(); //Conecta-se ao servidor MQTT
	time_message_generator(mqtt_data,num_of_wakes[0]); //Formata a data atual do ESP para dentro do array especificado
	printf("%s\n",mqtt_data);
	int resp = meshf_mqtt_publish("/data/esp32",strlen("/data/esp32"),mqtt_data,strlen(mqtt_data)); //Publica a data atual no topico /data/esp32
	printf("PUBLICACAO MQTT = %s\n",esp_err_to_name(resp));
	meshf_sleep_time(15000); //Bloqueia o ESP por 1 minuto
}

void define_root(void){
	uint8_t self_mac[6] = {0,};
	
	esp_mesh_fix_root(true);
    esp_read_mac(self_mac,ESP_MAC_WIFI_SOFTAP);
    if(self_mac[0]==root_node[0] && self_mac[1]==root_node[1] && 
       self_mac[2]==root_node[2] && self_mac[3]==root_node[3] && 
       self_mac[4]==root_node[4] && self_mac[5]==root_node[5]){
		wifi_config_t wifi_config = {0,};
		memcpy((uint8_t *)&wifi_config.sta.ssid,ROUTER_SSID,sizeof(ROUTER_SSID));
		memcpy((uint8_t *)&wifi_config.sta.password,ROUTER_PASSWORD,sizeof(ROUTER_PASSWORD));
		wifi_config.sta.channel = ROUTER_CHANNEL;
	    esp_mesh_set_parent(&wifi_config,NULL,MESH_ROOT,MESH_ROOT_LAYER);
    }
}

void app_main(void) {

	gpio_reset_pin(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    gpio_set_level(2, 1); //Acende o LED interno do ESP para mostrar que o ESP esta ligado
    
    char mqtt_data[150];

	uint8_t rx_mensagem[180] = {0,};
	int resp = 0;
	int sensor_seconds = 5;
    esp_sleep_wakeup_cause_t wakeUpCause;
	time_t now = 0;
    struct tm timeinfo = { 0 };
    meshf_init(); //Inicializa as configuracoes da rede MESH
    // define_root(); //Funcao para caso tenha o mac espeficicado, se torna o root
    wakeUpCause = esp_sleep_get_wakeup_cause();
    //Testa se o ESP acabou de sair do deepsleep ou nao
    switch (wakeUpCause){
		case ESP_SLEEP_WAKEUP_TIMER:;//Caso tenha saido por causa do temporizador
			int64_t before,after = 0;
			// sensor.trigger_pin = TRIGGER_GPIO;
            // sensor.echo_pin = ECHO_GPIO;
    		// ESP_ERROR_CHECK(ultrasonic_init(&sensor));
			before = esp_timer_get_time();
			num_of_wakes[0] = 0;
			ESP_LOGW("MESH_TAG","SENSOR STATUS: %s\nDISTANCE: %f\n",esp_err_to_name(measure_distance(&num_of_wakes[0])),num_of_wakes[0]);

    		time(&now); //Pega o tempo armazenado no RTC
			setenv("TZ", "UTC+3", 1); //Configura variaveis de ambiente com esse time zona
			tzset(); //Define a time zone
			localtime_r(&now, &timeinfo); //Reformata o horario pego do RTC

			printf("Faltam %d segundos para a transmissao\n",next_sleep_time(timeinfo,1));
			if((next_sleep_time(timeinfo,1) <= 10)){
				meshf_sleep_time(next_sleep_time(timeinfo,1)*1000);
				use_network(mqtt_data,rx_mensagem);
			}
			after = esp_timer_get_time();
			ESP_LOGE("MESH_TAG","Tempo ligado %d microssegundos",(int)(after-before));
			esp_sleep_enable_timer_wakeup(sensor_seconds*1000000);
			esp_deep_sleep_start(); //Coloca o esp em deep sleep
		break;
    	default: //Caso ele nao esteja saindo de um deepsleep
    		num_of_wakes[0] = 0;
    		
    		meshf_start(); //Inicializa a rede MESH
			meshf_start_sntp(); //Se conecta ao server SNTP e atualiza o seu relogio RTC (caso seja root)
			meshf_rx(rx_mensagem); //Seta o buffer para recepcao das mensagens
			meshf_start_mqtt(); //Conecta-se ao servidor MQTT
			meshf_asktime(); //Pede ao noh root pelo horario atual que foi recebido pelo SNTP (caso nao seja root)
			time_message_generator(mqtt_data,num_of_wakes[0]); //Formata a data atual do ESP para dentro do array especificado
			printf("%s\n",mqtt_data);

    		time(&now); //Pega o tempo armazenado no RTC
			setenv("TZ", "UTC+3", 1); //Configura variaveis de ambiente com esse time zona
			tzset(); //Define a time zone
			localtime_r(&now, &timeinfo); //Reformata o horario pego do RTC

			int awake_until = next_sleep_time(timeinfo,1); //Calcula até quanto tempo para XX:AA:00. Sendo AA os minutos multiplos de 5 mais prox.
			printf("Vai continuar acordado por %d segundos\n",awake_until);
			meshf_sleep_time(awake_until*1000); //Bloqueia o fluxo do codigo até que o horario estipulado anteriormente seja atingido
			time_message_generator(mqtt_data,num_of_wakes[0]); //Formata a data atual do ESP para dentro do array especificado
			printf("%s\n",mqtt_data);
			resp = meshf_mqtt_publish("/data/esp32",strlen("/data/esp32"),mqtt_data,strlen(mqtt_data)); //Publica a data atual no topico /data/esp32
			printf("PUBLICACAO MQTT = %s\n",esp_err_to_name(resp));

			meshf_sleep_time(15000); //Bloqueia o ESP por 1 minuto

			esp_sleep_enable_timer_wakeup(sensor_seconds*1000000); //Ativa o esp para acordar depois que o tempo definido anteriormente ter passado
			esp_wifi_stop();
			esp_deep_sleep_start(); //Coloca o esp em deep sleep
		break;
	}
}