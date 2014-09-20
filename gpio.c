
#include <stdio.h>
#include <wiringPi.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int ret;
    int value;

    if(argc<2) {
        printf("usage: %s value\n", argv[0]);
        exit(1);
    }

    value = atoi(argv[1]);

    ret = wiringPiSetup();
    printf("setup %d\n", ret);

    // physical pin #24 on Bananpi
    // man gpio for pin mapping.
    pinMode(6, 1);  // set to output mode

    printf("set pin %d\n", value);
    digitalWrite(6, value);

    return 0;
}

