#include <string.h>
#include <esp_system.h>
#include <time.h>
#include <sys/time.h>


//Define the following macro to 1 if the sensor is going to be used
#define DHT 0
#define ULTRASONIC 0
#define GPS 0

#if ULTRASONIC
#include <ultrasonic.h>
#endif //ULTRASONIC

#if DHT
#include "DHT.h"
#endif //DHT

#if GPS
#include "aos_gps_c.h"
#endif //GPS

#include "mesh_framework.h"

#include <driver/adc.h>
#include "driver/gpio.h"
#include "esp_sleep.h"

#define MAX_LAYERS CONFIG_MAX_LAYERS
#define ROUTER_CHANNEL CONFIG_ROUTER_CHANNEL
#define ROUTER_SSID CONFIG_ROUTER_SSID
#define ROUTER_PASSWORD CONFIG_ROUTER_PASSWORD
#define MAX_CLIENTS CONFIG_MAX_CLIENTS

#define MAX_DISTANCE_CM 300 // 5m max
#define TRIGGER_GPIO GPIO_NUM_23
#define ECHO_GPIO GPIO_NUM_22

RTC_DATA_ATTR float num_of_wakes[10];
static const uint8_t root_node[] = {0x80,0x7d,0x3a,0xb7,0xc8,0x19};
#if ULTRASONIC
static ultrasonic_sensor_t sensor;
#endif
#if GPS
static uBloxGPS_t *gps = NULL;
#endif

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
	cJSON_AddNumberToObject(json_mqtt,"rssi", mesh_parent.rssi);
	cJSON_AddNumberToObject(json_mqtt,"distance", value);
	cJSON_AddStringToObject(json_mqtt,"time", final_message);

	#if DHT

	setDHTgpio(GPIO_NUM_4);
    ESP_LOGI("TAG", "Starting DHT Task\n\n");

    ESP_LOGI("TAG", "=== Reading DHT ===\n");
    
    int ret = readDHT();

    errorHandler(ret);

    cJSON_AddNumberToObject(json_mqtt, "humidity", getHumidity());  
	cJSON_AddNumberToObject(json_mqtt, "temperature", getTemperature());

	#endif //DHT

	#if GPS
	cJSON_AddNumberToObject(json_mqtt,"lat", (-1)*gps_GetLat(gps));
	cJSON_AddNumberToObject(json_mqtt,"lon", (-1)*gps_GetLng(gps));

	#endif //GPS

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
	assert(distance != NULL);
	
    #if ULTRASONIC 

    esp_err_t err = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, distance);
    *distance = (*distance)*100.0;

	#else

    esp_err_t err = ESP_OK;
    //Generates a number between 0 MAX_DISTANCE_CM
    *distance = (float)(esp_random()/(UINT32_MAX/MAX_DISTANCE_CM + 1)); 

    #endif
    
    return err;
    
    
}

void use_network(char mqtt_data[], uint8_t rx_mensagem[]){
	esp_err_t mesh_on = meshf_start(45000/portTICK_PERIOD_MS); //Inicializa a rede MESH
	if (mesh_on != ESP_OK){
		esp_restart();
	}
	ESP_ERROR_CHECK(meshf_rx(rx_mensagem)); //Seta o buffer para recepcao das mensagens
	meshf_start_mqtt(); //Conecta-se ao servidor MQTT

	time_message_generator(mqtt_data,num_of_wakes[0]); //Formata a data atual do ESP para dentro do array especificado
	printf("%s\n",mqtt_data);
	esp_err_t resp = meshf_mqtt_publish("/data/esp32",strlen("/data/esp32"),mqtt_data,strlen(mqtt_data)); //Publica a data atual no topico /data/esp32
	printf("PUBLICACAO MQTT = %s\n",esp_err_to_name(resp));

	if(esp_mesh_is_root()){ //Routine to wait for a child to connect before going back to deepsleep (waste of time in this current implementation)
		if(esp_mesh_get_total_node_num() <= 1){
			ESP_LOGI("TAG","Waiting for a node to connect as a root");
			meshf_sleep_time(20000); //Bloqueia o ESP por 1 minuto
		}
	}else{
		if (esp_mesh_get_total_node_num() <= 1){
			ESP_LOGI("TAG","Waiting for a node to connect as a commum node");
			meshf_sleep_time(20000); //Bloqueia o ESP por 1 minuto	
		}
	}

	if(esp_mesh_get_total_node_num() > 1){
		ESP_LOGI("TAG","Waiting for a node to disconnect");
		meshf_sleep_time(1000); //Bloqueia o ESP por 1 minuto
	}

	meshf_sleep_time(5000); //Bloqueia o ESP por 1 minuto
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
			#if ULTRASONIC
			sensor.trigger_pin = TRIGGER_GPIO;
            sensor.echo_pin = ECHO_GPIO;
    		ESP_ERROR_CHECK(ultrasonic_init(&sensor));
    		#endif
			before = esp_timer_get_time();
			num_of_wakes[0] = 0;
			ESP_LOGW("MESH_TAG","SENSOR STATUS: %s\nDISTANCE: %f\n",esp_err_to_name(measure_distance(&num_of_wakes[0])),num_of_wakes[0]);

			time(&now); //Pega o tempo armazenado no RTC
			setenv("TZ", "UTC+3", 1); //Configura variaveis de ambiente com esse time zona
			tzset(); //Define a time zone
			localtime_r(&now, &timeinfo); //Reformata o horario pego do RTC
			
			int next_sleep = next_sleep_time(timeinfo,1);
			printf("Faltam %d segundos para a transmissao\n",next_sleep);
			if((next_sleep_time(timeinfo,1) <= 10)){
				meshf_sleep_time(next_sleep_time(timeinfo,1)*1000);
				use_network(mqtt_data,rx_mensagem);
			}
			after = esp_timer_get_time();
			ESP_LOGE("MESH_TAG","Tempo ligado %d microssegundos",(int)(after-before));
			esp_sleep_enable_timer_wakeup(sensor_seconds*1000000);
			meshf_stop();
			esp_deep_sleep_start(); //Coloca o esp em deep sleep
		break;
    	default: //Caso ele nao esteja saindo de um deepsleep
    		num_of_wakes[0] = 0;
    		
    		esp_err_t mesh_on = meshf_start(45000/portTICK_PERIOD_MS); //Inicializa a rede MESH
    		if (mesh_on != ESP_OK){
				esp_restart();
			}

			meshf_start_sntp(); //Se conecta ao server SNTP e atualiza o seu relogio RTC (caso seja root)
			ESP_ERROR_CHECK(meshf_rx(rx_mensagem)); //Seta o buffer para recepcao das mensagens
			meshf_start_mqtt(); //Conecta-se ao servidor MQTT
			ESP_ERROR_CHECK(meshf_asktime(45000/portTICK_PERIOD_MS)); //Pede ao noh root pelo horario atual que foi recebido pelo SNTP (caso nao seja root)
			time_message_generator(mqtt_data,num_of_wakes[0]); //Formata a data atual do ESP para dentro do array especificado
			printf("%s\n",mqtt_data);

    		time(&now); //Pega o tempo armazenado no RTC
			setenv("TZ", "UTC+3", 1); //Configura variaveis de ambiente com esse time zona
			tzset(); //Define a time zone
			localtime_r(&now, &timeinfo); //Reformata o horario pego do RTC

			#if GPS
			gps = init_Gps(UART_NUM_2, GPIO_NUM_17, GPIO_NUM_16);

			while(true)
			{
			#endif //GPS

			int awake_until = next_sleep_time(timeinfo,1); //Calcula até quanto tempo para XX:AA:00. Sendo AA os minutos multiplos de 5 mais prox.
			ESP_LOGW("MESH_TAG","Vai continuar acordado por %d segundos\n",awake_until);
			meshf_sleep_time(awake_until*1000); //Bloqueia o fluxo do codigo até que o horario estipulado anteriormente seja atingido
			measure_distance(&num_of_wakes[0]);
			time_message_generator(mqtt_data,num_of_wakes[0]); //Formata a data atual do ESP para dentro do array especificado
			printf("%s\n",mqtt_data);
			resp = meshf_mqtt_publish("/data/esp32",strlen("/data/esp32"),mqtt_data,strlen(mqtt_data)); //Publica a data atual no topico /data/esp32
			printf("PUBLICACAO MQTT = %s\n",esp_err_to_name(resp));

			#if GPS
			} //ending Bracket of while(true) in case of GPS is 1s
			#endif
			ESP_LOGW("MESH_TAG","Vai continuar acordado por 60 segundos\n");
			meshf_sleep_time(60000); //Bloqueia o ESP por 1 minuto

			esp_sleep_enable_timer_wakeup(sensor_seconds*1000000); //Ativa o esp para acordar depois que o tempo definido anteriormente ter passado
			esp_wifi_stop();
			esp_deep_sleep_start(); //Coloca o esp em deep sleep
		break;
	}
}