#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int *pointer2 = NULL;

void teste(int *pointer){
	pointer2 = pointer;
}

int muda_valor(int valor){
	int abacaxi2[8] = {0,};
	for (int i = 0; i < ((int)sizeof(abacaxi2)/4);i++){
		abacaxi2[i] += (valor + i);
	}

	memcpy(pointer2,abacaxi2,sizeof(abacaxi2));

	return sizeof(abacaxi2);
}

void copy(int *pointer,int tamanho, int *final){
	for(int i = 0;i < tamanho/4; i++){
		final[i] = pointer[i];
	}
}


int main(){
	int abacaxi[1460] = {0,};
	teste(abacaxi);
	int tam = muda_valor(13);
	int final[tam/4];
	copy(abacaxi,tam,final);
	for (int i = 0;i < sizeof(final)/4; i++){
		printf("%d\n",final[i]);
	} 
}