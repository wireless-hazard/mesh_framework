#include <stdio.h>
#include "calc_time.h"
#include <time.h>

int main(){
	time_t now = 0;
    struct tm timeinfo = { 0 };

    time(&now); //Pega o tempo armazenado no RTC
	// setenv("TZ", "UTC+3", 1); //Configura variaveis de ambiente com esse time zona
	tzset(); //Define a time zone
	localtime_r(&now, &timeinfo); //Reformata o horario pego do RTC
	printf("%d\n",next_sleep_time(timeinfo,2));
}