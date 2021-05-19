#include "calc_time.h"
#include <time.h>

int next_sleep_time(const struct tm timeinfo,int fixed_gap){ //Recebe o valor em minutos e calcula em quantos segundos o ESP devera acordar, considerando o inicio em uma hora exata.
	if (fixed_gap <= 0){ //Se o valor do delay for de 0 minutos, retorna 0 minutos sem fazer nenhum calculo
		return 0;
	}

	int minutes = timeinfo.tm_min; //Pega o valor dos minutos atuais do RTC
	int rouded_minutes = (minutes / fixed_gap) * fixed_gap; //Transforma o valor dos minutos atuais do RTC o multiplo anterior de fixed_gap 
	int next_minutes = rouded_minutes + fixed_gap - 1; //Calcula qual o prox valor de minutos que seja multiplo de fixed_gap 
	int next_seconds = 60 - timeinfo.tm_sec; //Calcula o valor de segundos até que seja o minuto exato: XX:XX:00

	return ((next_minutes - minutes)*60 + next_seconds); //Retorna o tempo em segundos até que o ESP tenha seu RTC a XX:AA:00 sendo AA o prox valor em minutos multiplo de fixed_gap
}