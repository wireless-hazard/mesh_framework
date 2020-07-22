#include <stdio.h>
#include <string.h>

int main(){
	int size = 10;
	int vetor[10] = {0,};
	int a[] = {1,2,3};
	memcpy(vetor + (sizeof(vetor)-sizeof(a))/sizeof(int),a,sizeof(a));
	for (int i = 0; i < sizeof(vetor)/sizeof(int);i++){
		printf("%d\n",vetor[i] );
	}

	char sim[45];
	int inteiros[4] = {5,100,13,2};
	char exnteiros[4*3+(4)];
	char total[45+(4*3)+(4)];
	for (int i = 0; i < sizeof(inteiros)/sizeof(int); ++i){
		if (i == 0){
			sprintf(exnteiros,"%.3d;",inteiros[i]);
		}else if (i == sizeof(inteiros)/sizeof(int) - 1){
			sprintf(exnteiros + 3*i+i,"%.3d",inteiros[i]);
		}else{
			sprintf(exnteiros + 3*i+i,"%.3d;",inteiros[i]);
		}

	}

	printf("%s : tam1: %d\n",exnteiros,(int)sizeof(exnteiros));
	strcpy(sim,"a4:cf:12:75:21:30;80:7d:3a:b7:c8:19;-042;02;");
	printf("%s : tam2: %d\n",sim,(int)sizeof(sim));
	sprintf(total,"%s",sim);
	sprintf(total + (int)sizeof(sim)-1,"%s",exnteiros);

	printf("%s : tam3: %d\n",total,(int)sizeof(total));
}