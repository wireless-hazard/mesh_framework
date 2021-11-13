#include "stdio.h"
#include "stdint.h"

#define N_SAMPLES 16

int main(){
	float floating_samples[N_SAMPLES/4] = {1.8712,1,1,1};
	uint8_t *int_samples = NULL;
	int_samples = (uint8_t *)&floating_samples;
	for (int i = 0; i < N_SAMPLES; i++)
	{
		printf("%d\n", int_samples[i]);
	}

	float *float_samples = (float *)int_samples;

	for (int i = 0; i < N_SAMPLES/4; i++)
	{
		printf("%f\n", float_samples[i]);
	}
	// float floating = 2555.55555;
	// int8_t floater =  *(int8_t *)&floating;
	// printf("%d\n", floater); 
}