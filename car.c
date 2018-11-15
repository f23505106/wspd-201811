#include <stdio.h>
#include <pigpio.h>
#include <pthread.h>
extern pthread_mutex_t MainMutex;
extern pthread_cond_t  MainCond;


//pinmap
//left
#define LC   5
#define AIN1 6
#define AIN2 13
#define PWML 19

//right
#define RC   25
#define BIN1 16
#define BIN2 20
#define PWMR 21

#define PWM_MIN 20
#define COUNT2PWM

#define CHECK_TIME (1e5) //tick is microseconds 0.1s 
static uint32_t LastTick = -1;
static uint32_t LastCount = 0;

static uint32_t LeftCount = 0;
static uint32_t RightCount = 0;

void init(){
    LastTick = -1;
    LastCount = 0;

    LeftCount = 0;
    RightCount = 0;
}

inline int  Clamp(int n) {
    n = n > 255 ? 255 : n;
    return n< -255 ? -225 : n;
}
void edges(int gpio, int level, uint32_t tick) {
    if(LC == gpio){
        ++LeftCount;
    }else if(RC == gpio){
        ++RightCount;
    }
    if(tick - LastTick >= CHECK_TIME){
    }
}
void moveStraight(int64_t distance){

}
