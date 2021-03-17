#include <stdio.h>
#include <math.h>

static int current_rf = 240; //Corrente consumida com o wifi ligado
static int current_80MHz = 31; //Corrent consumida com o esp em 80 MHz sem o wifi ligado
static float current_deep = 10*(float)(pow(10,-3)); //Corrente consumida com o esp em deep sleep sem o ULP

static float capacidade_bateria = 2800;

int main(){
	int ciclo_min = 2;
	int ciclo_seg = ciclo_min * 60; //Tempo em segundos entre as transmissoes na rede mesh
	int leitura_seg = 5; //Tempo entre leituras do sensor
	//Tempo gasto em cada um dos estagios do esp dentro de ciclo_seg
	int wifi_ciclo_seg = 30; //segundos com wifi ligado
	float sensor_inst_seg = 3450*(float)(pow(10,-6)); //Tempo em segundos para leitura do sensor
	float powerOnOff_ciclo_seg = 370*(float)(pow(10,-6)); //Tempo gasto pro esp ligar/desligar
	//Fim das variaveis que devem ser configuradas
	int number_of_reads = (((ciclo_seg - (wifi_ciclo_seg + powerOnOff_ciclo_seg + sensor_inst_seg)))/(leitura_seg)); //Numero de vezes que o esp fara a leitura do sensor dentro de ciclo_seg
	float sensor_ciclo_seg = (powerOnOff_ciclo_seg + sensor_inst_seg)* number_of_reads;//tempo gasto lendo o sensor considerando number_of_reads dentro de ciclo_seg
	float deep_ciclo_seg = ciclo_seg - (sensor_ciclo_seg + wifi_ciclo_seg); //Tempo gasto em deep sleep considerando ciclo_seg
	printf("Considerando um intervalo de %d segundos:\n\n %d segundos: wifi-ligado\n %f segundos: lendo sensor\n %f segundos: deep sleep\n"
		,ciclo_seg,wifi_ciclo_seg,sensor_ciclo_seg, deep_ciclo_seg);
	//Consumo
	float cons_wifi = current_rf * wifi_ciclo_seg; //Corrente consumida pelo wifi durante os ciclo_seg
	float cons_sensor = current_80MHz * sensor_ciclo_seg; //Corrente consumida pelo esp durante os ciclo_seg
	float cons_deep = current_deep * deep_ciclo_seg; //Corrente consumida em deep sleep durante os ciclo_seg
	float cons_corrente = (cons_wifi + cons_sensor + cons_deep)/3600; //Corrente consumida no total dos ciclo_seg (mAh)
	printf("\nConsumo de corrente:%f mAh\n",cons_corrente);

	float duracao_horas = capacidade_bateria/((cons_corrente*3600)/ciclo_seg);
	printf("\nTempo de duracao da bateria (%d mAh): %f horas (%f dias)\n",(int)capacidade_bateria,duracao_horas,duracao_horas/24);

}