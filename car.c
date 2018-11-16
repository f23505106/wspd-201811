#include <stdio.h>
#include <pigpio.h>
#include <pthread.h>

extern pthread_mutex_t MainMutex;
extern pthread_cond_t  MainCond;


//pinmap
//left
#define LC  0// 5
#define AIN1 1//6
#define AIN2 4//13
#define PWML 17//19

//right
#define RC   11//26
#define BIN1 14//16
#define BIN2 15//20
#define PWMR 18//21

#define TURN_90_COUNT 100
#define TURN_180_COUNT 200

#define CHECK_TIME (100000) //tick is microseconds 0.1s 
#define DEBOUNC_TIME 1000
static uint32_t LastTick = -1;

static uint32_t LeftCount = 0;
static uint32_t RightCount = 0;
static uint8_t LeftPwm;
static uint8_t RightPwm;
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
            printf("left over need go back\n");
            Ldir = ! LTartgetDir;
            leftDir(Ldir);
            diff = -diff;
        }
        LeftPwm = LeftPwm < diff2pwm(diff)?(++LeftPwm):(--LeftPwm);//todo adjust
        gpioPWM(PWML,LeftPwm);
    }else if(RIGHT == w){
        if(diff < 0){
            printf("right over need go back\n");
            Rdir = ! RTartgetDir;
            rightDir(Rdir);
            diff = -diff;
        }
        RightPwm = RightPwm < diff2pwm(diff)?(++RightPwm):(--RightPwm);//todo adjust
        gpioPWM(PWMR,RightPwm);
    }else{
        printf("updatePwm error\n");
    }
    printf("LTagetDistance:%d LeftCount:%d lpwm:%d RTagetDistance:%d RightCount:%d rpwm:%d\n",LTagetDistance,LeftCount,LeftPwm,RTagetDistance,RightCount,RightPwm);
}
void updateSpeed(){
    int ldiff = LTagetDistance - LeftCount;
    int rdiff = RTagetDistance - RightCount;
    if(0 == ldiff && 0 == rdiff){
        printf("arrive postion\n");
        wakeMainThread();
        return;
    }
    if(ldiff == 0){
        printf("left arrived\n");
    }else{
        updatePwm(LEFT,ldiff);
    }
    if(rdiff == 0){
        printf("right arrived\n");
    }else{
        updatePwm(RIGHT,rdiff);
    }
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
            printf("gpio:%d timeout left pwm:%d right pwm:%d min pwm %d\n",gpio,LeftPwm,RightPwm,PwmMin);
            ++PwmMin;
            if(LC == gpio){
                if(LTagetDistance > LeftCount){
                    gpioPWM(PWML,++LeftPwm);
                }else if(LTagetDistance < LeftCount){
                    gpioPWM(PWML,--LeftPwm);
                }
            }
            if(RC == gpio){
                if(RTagetDistance > RightCount){
                    gpioPWM(PWMR,++RightPwm);
                }else if(RTagetDistance < RightCount){
                    gpioPWM(PWMR,--RightPwm);
                }
            }
        }
        return;
    }
    if(0 ==level){//falling edge
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
}
void initOnce(){
    if(InitOnceFlag){
        return;
    }
    printf("init once\n");
    InitOnceFlag = 1;
    gpioSetMode(LC, PI_INPUT);
    gpioSetPullUpDown(LC, PI_PUD_UP);
    gpioGlitchFilter(LC,DEBOUNC_TIME);
    gpioSetAlertFunc(LC, edges);
    gpioSetMode(AIN1, PI_OUTPUT);
    gpioSetMode(AIN2, PI_OUTPUT);
    gpioSetMode(PWML, PI_OUTPUT);
    gpioSetWatchdog(LC, CHECK_TIME>>0x3);

    gpioSetMode(RC, PI_INPUT);
    gpioSetPullUpDown(RC, PI_PUD_UP);
    gpioGlitchFilter(RC,DEBOUNC_TIME);
    gpioSetAlertFunc(RC, edges);
    gpioSetMode(BIN1, PI_OUTPUT);
    gpioSetMode(BIN2, PI_OUTPUT);
    gpioSetMode(PWMR, PI_OUTPUT);
    gpioSetWatchdog(RC, CHECK_TIME>>0x3);
}
void init(int64_t lDistance,int64_t rDistance){
    printf("init ldis:%d,rdis:%d\n",lDistance,rDistance);
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
        RTagetDistance = rDistance;
        Rdir = RTartgetDir = FW;
    }
    leftDir(Ldir);
    rightDir(Rdir);

    LastTick = -1;

    LeftCount = 0;
    RightCount = 0;
    LeftPwm = PwmMin;
    RightPwm = PwmMin;
    initOnce();
    //start to move
    printf("start to move\n");
    gpioPWM(PWML,PwmMin);
    gpioPWM(PWMR,PwmMin);

}
int moveStraight(int64_t distance){
    printf("move straight %d\n",distance);
    if(0 == distance){
        printf("distance is 0,just return\n");
        wakeMainThread();
        return -1;
    }
    init(0,distance);
    return 0;
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

