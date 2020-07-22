#include <stdio.h>
int *p_g = NULL;
int x = 5;

void teste(int *p){
	p_g = p;
}
void soma(){
	*p_g = x;
}
int main(){
	int a = 2;
	teste(&a);
	soma();
	printf("%d\n",a);
}