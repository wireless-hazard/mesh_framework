#include <stdio.h>
#include <string.h>

void muda_valor(void *pvParameters){
	int *pointer;
	pointer = (int *)pvParameters;
	for (int i = 0; i < 8;i++){
		pointer[i] += (i);
	}
}

void teste(int *pointer){
	muda_valor((void *)pointer);
}

int main(){
	int abacaxi[8] = {0,1,2,3,};
	teste(abacaxi);
	for (int i = 0;i < sizeof(abacaxi)/4; i++){
		printf("%d\n",abacaxi[i]);
	} 
}