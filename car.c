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
#define RC   26
#define BIN1 16
#define BIN2 20
#define PWMR 21

#define TURN_90_COUNT 100
#define TURN_180_COUNT 200

#define CHECK_TIME (100000) //tick is microseconds 0.1s 
static uint32_t LastTick = -1;

static uint32_t LeftCount = 0;
static uint32_t RightCount = 0;

uint8_t PwmMin = 20;

enum WHICH{LEFT=0,RIGHT};

enum DIR{FW=0,BW};

static enum DIR LTartgetDir;
static enum DIR RTartgetDir;
static uint32_t LTagetDistance;
static uint32_t RTagetDistance;

static enum DIR Ldir;
static enum DIR Rdir;
static int InitOnceFlag = 0;
void wakeMainThread(){
    pthread_mutex_lock( &MainMutex );
    pthread_cond_signal( &MainCond );
    pthread_mutex_unlock( &MainMutex );
}
uint8_t  clamp(int n) {
    n = n > 255 ? 255 : n;
    return n< PwmMin ? PwmMin : n;
}
void leftDir(enum DIR dir){
    gpioWrite(AIN1, dir);
    gpioWrite(AIN2, !dir);
}
void rightDir(enum DIR dir){
    gpioWrite(BIN1, dir);
    gpioWrite(BIN2, !dir);
}

uint8_t diff2pwm(int diff){
    diff>>=0x1;//todo adjust
    return clamp(diff);
}
void updatePwm(enum WHICH w,int diff){
    if(LEFT == w){
        if(diff < 0){
            Ldir = ! LTartgetDir;
            leftDir(Ldir);
            diff = -diff;
        }
        int pwm = gpioGetPWMdutycycle(PWML);
        pwm = pwm < diff2pwm(diff)?(++pwm):(--pwm);//todo adjust
        gpioPWM(PWML,pwm);
    }else if(RIGHT == w){
        if(diff < 0){
            Rdir = ! RTartgetDir;
            rightDir(Rdir);
            diff = -diff;
        }
        int pwm = gpioGetPWMdutycycle(PWMR);
        pwm = pwm < diff2pwm(diff)?(++pwm):(--pwm);//todo adjust
        gpioPWM(PWMR,pwm);
    }else{
        printf("updatePwm error\n");
    }
}
void updateSpeed(){
    int ldiff = LTagetDistance - LeftCount;
    int rdiff = RTagetDistance - RightCount;
    if(0 == ldiff && 0 == rdiff){
        printf("arrive postion\n");
        wakeMainThread();
        return;
    }
    updatePwm(LEFT,ldiff);
    updatePwm(RIGHT,rdiff);
}
/*
Parameter   Value    Meaning

GPIO        0-31     The GPIO which has changed state

level       0-2      0 = change to low (a falling edge)
                     1 = change to high (a rising edge)
                     2 = no level change (a watchdog timeout)

tick        32 bit   The number of microseconds since boot
                     WARNING: this wraps around from
                     4294967295 to 0 roughly every 72 minutes
*/
void edges(int gpio, int level, uint32_t tick) {
    if(PI_TIMEOUT == level){
        if((LTagetDistance == LeftCount) && (RTagetDistance == RightCount)){
            printf("timeout arrived\n");
            wakeMainThread();
        }else{
            uint8_t left = gpioGetPWMdutycycle(PWML);
            uint8_t right = gpioGetPWMdutycycle(PWMR);
            printf("gpio:%d timeout left pwm:%d right pwm:%d min pwm %d\n",gpio,left,right,PwmMin);
            ++PwmMin;
            gpioPWM(PWML,PwmMin);
            gpioPWM(PWMR,PwmMin);
        }
        return;
    }
    if(LC == gpio){
        LeftCount += (Ldir == LTartgetDir ? 1:-1);;
    }else if(RC == gpio){
        RightCount += (Rdir == RTartgetDir ? 1:-1);;
    }else{
        //maybe other funs
        return;
    }
    if(tick - LastTick >= CHECK_TIME){
        updateSpeed();
        LastTick = tick;
    }
}
void initOnce(){
    if(InitOnceFlag){
        return;
    }
    InitOnceFlag = 1;
    gpioSetMode(LC, PI_INPUT);
    gpioSetPullUpDown(LC, PI_PUD_UP);
    gpioSetAlertFunc(LC, edges);
    gpioSetMode(AIN1, PI_OUTPUT);
    gpioSetMode(AIN2, PI_OUTPUT);
    gpioSetMode(PWML, PI_OUTPUT);
    gpioSetWatchdog(LC, CHECK_TIME<<0x3);

    gpioSetMode(RC, PI_INPUT);
    gpioSetPullUpDown(RC, PI_PUD_UP);
    gpioSetAlertFunc(RC, edges);
    gpioSetMode(BIN1, PI_OUTPUT);
    gpioSetMode(BIN2, PI_OUTPUT);
    gpioSetMode(PWMR, PI_OUTPUT);
    gpioSetWatchdog(RC, CHECK_TIME<<0x3);
}
void init(int64_t lDistance,int64_t rDistance){
    if(lDistance < 0){
        LTagetDistance = -lDistance;
        Ldir = LTartgetDir = BW;
    }else{
        LTagetDistance = lDistance;
        Ldir = LTartgetDir = FW;
    }
    if(rDistance < 0){
        RTagetDistance = -rDistance;
        Rdir = RTartgetDir = BW;
    }else{
        RTagetDistance = -rDistance;
        Rdir = RTartgetDir = FW;
    }
    leftDir(Ldir);
    rightDir(Rdir);

    LastTick = -1;

    LeftCount = 0;
    RightCount = 0;
    initOnce();
    //start to move
    gpioPWM(PWML,PwmMin);
    gpioPWM(PWMR,PwmMin);

}
void moveStraight(int64_t distance){
    printf("move straight %d",distance);
    if(0 == distance){
        printf("distance is 0,just return");
        wakeMainThread();
        return;
    }
    init(distance,distance);
}

void turnLeft(){
    init(TURN_90_COUNT,-(TURN_90_COUNT));
}
void turnRight(){
    init(-(TURN_90_COUNT),TURN_90_COUNT);
}
void turnBack(){
    init(TURN_180_COUNT,-TURN_180_COUNT);
}

