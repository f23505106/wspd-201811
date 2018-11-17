#include<pigpio.h>

#define LC   5
#define AIN1 6
#define AIN2 13
#define PWML 19

//right
#define RC   26
#define BIN1 16
#define BIN2 20
#define PWMR 21
int main(){
    gpioPWM(PWML,0);
    gpioPWM(PWMR,0);
    return 0;
}
