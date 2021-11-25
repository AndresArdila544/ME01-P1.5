
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.cpp"


int main(void){
    int a = 12;
    int b = 25;
    for(int i = 0; i <100;i++){
        float media = 8.0;
        float log_ = log(lcgrand(2));
        float n_generado = -media*log_;
        printf("Log: %f Gen: %f \n",log_,n_generado);
    }
    
    return 0;
}