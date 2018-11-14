/*e.c

gcc -o pulse pulse.c -lpigpio -lrt -lpthread

sudo ./pulse
*/

#include <stdio.h>
#include <pigpio.h>
int main(int argc, char *argv[])
{
    double start;
    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "pigpio initialisation failed\n");
        return 1;
    }
    /* Set GPIO modes */
//    gpioSetMode(4, PI_OUTPUT);
    gpioSetMode(17, PI_OUTPUT);
//    gpioSetMode(18, PI_OUTPUT);
//    gpioSetMode(23, PI_INPUT);
//    gpioSetMode(24, PI_OUTPUT);

    /* Start 1500 us servo pulses on GPIO4 */
//    gpioServo(4, 1500);

    /* Start 75% dutycycle PWM on GPIO17 */
//    gpioPWM(17, 192); /* 192/255 = 75% */

    start = time_time();
    int v = 255;
    int dir = -1;
    while ((time_time() - start) < 60.0)
    {
       printf("current v:%d\n",v);
       gpioPWM(17, v); /* 192/255 = 75% */
       time_sleep(0.005);
       if(0 == v){
        dir = 1;
       }else if(255 == v){
        dir = -1;
       }
       v+=dir;

    }

    /* Stop DMA, release resources */
    gpioTerminate();

    return 0;
}
