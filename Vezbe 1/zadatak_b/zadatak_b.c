#include <stdio.h>

int x;
int y;
int rezultat;


int div(int x, int y){
	if(y==0){
		return -24;
}else
{
	return x/y;
}
}

void div_print(int rezultat){
		if(rezultat == -24){
			
			
			printf("Deljenje sa nulom nije dozvoljeno! \n");
		
		}
		else
			{
			printf("Rezultat deljenja je: %d \n", rezultat);
		}
}

void main(){
	
	
	int rezultat1 = div(21,7);
	
	int rezultat2 = div(5,0);

	div_print(rezultat1);
	div_print(rezultat2);
		
	
}