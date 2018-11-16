#include <stdio.h>
#include <pthread.h>
#include<pigpio.h>
#include "car.h"
pthread_mutex_t MainMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  MainCond  = PTHREAD_COND_INITIALIZER;
int main(int argc, char *argv[])
{
    if (gpioInitialise() < 0){
        fprintf(stderr, "pigpio initialisation failed\n");
        return 1;
    }
    //do work on pigpio pthAlertThread main thread just wait
    //Todo use config file to describe movement
    pthread_mutex_lock( &MainMutex );
    pthread_cond_wait( &MainCond, &MainMutex);
    pthread_mutex_unlock( &MainMutex);
    printf("just log main thread was notified,bye\n");

    gpioTerminate();
    return 0;
}
