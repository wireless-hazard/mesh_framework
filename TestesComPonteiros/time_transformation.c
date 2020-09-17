#include <stdio.h>

int main(){
	
	int vetor[14];
	int tempo = 1600320023;
	int factor = 1000000000;
	int index = 13;
	vetor[0] = 0;
	vetor[1] = 0;
	vetor[2] = 0;
	vetor[3] = 0;
	printf("%d\n",sizeof(vetor)/4);
	while(index >= 4){
		vetor[index] = tempo/factor;
		printf("Printando os valores:\n vetor[index] = %d \n tempo = %d \n factor = %d \n index = %d \n",vetor[index],tempo,factor,index);
		tempo = tempo - (vetor[index]*factor);
		factor = factor/10;
		index = index - 1;
	}
    printf("\nVetor final: [ ");
    for(int i = 13; i >= 4;i--){
    	printf("%d ",vetor[i]);
    }
    printf("]\n");
}