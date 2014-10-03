
#include <stdio.h>
#include <wiringPi.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <time.h>

int main(int argc, char **argv)
{
    int ret;
    int fd;

    char clr[] = {0x80};
    char backlight[] = {0x8A, 10};
    char showchar[] = {0x07, 10, 10, 'A'};
    char setfont[] = {0x81, 0xF0}; // 8x16, black character
    char circle[] = {0x06, 64, 32, 10};
    char square[] = {0x04, 5, 32, 20, 42};

    #define CMD(x) { x, sizeof(x) }
    struct { char *cmd; int len; } 
        cmd[] = {
                    CMD(clr),
                    CMD(backlight),
                    CMD(setfont),
                    CMD(showchar),
                    CMD(circle),
                    CMD(square),
                };

    char hello[] = "Hello, AVerMedia";
    ssize_t sz;
    uint8_t mode = SPI_CPHA|SPI_CPOL; // clock high active, latch on rising edge
    uint8_t bits = 8;
    uint32_t speed = 500000;
    int i;

    ret = wiringPiSetup();
    if(ret) {
        printf("wiringPi set up failed: %d\n", ret);
        exit(1);
    }

    // physical pin #24 on Bananpi
    // man gpio for pin mapping.
    pinMode(6, 1);  // set to output mode

    // reset LCD
    digitalWrite(6, 1);
    usleep(1*1000);
    digitalWrite(6, 0);
    usleep(4*1000); // >2ms
    digitalWrite(6, 1);
    usleep(20*1000); // >10ms

    fd = open("/dev/spidev0.0", O_RDWR);
    if(fd<0) {
        printf("open device failed: %s\n", strerror(errno));
    }

    // set SPI mode
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
        printf("cannot set SPI mode: %s\n", strerror(errno));
        exit(1);
    } 

    // set bits per word
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
        printf("cannot set bits per word: %s\n", strerror(errno));
        exit(1);
    }

    // set speed
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
        printf("cannot set speed: %s\n", strerror(errno));
        exit(1);
    }

    for(i=0; i<sizeof(cmd)/sizeof(cmd[0]); ++i) {
        printf("execute cmd %d\n", i);

        sz = write(fd, cmd[i].cmd, cmd[i].len);
        if(sz<0) {
            printf("write cmd[%d]-%d failed: %s\n", i, cmd[i].len, strerror(errno));
            break;
        }

        usleep(1*1000); // no busy flag, 1ms might be safe
    }

    // print hello string
    for(i=0; i<strlen(hello); ++i) {
        char cmd[] = { 0x07, 0, 0, 0};
        cmd[1] = i*8;
        cmd[3] = hello[i];
        sz = write(fd, cmd, sizeof(cmd));

        if(sz<0) {
            printf("write hello[%d] failed: %s\n", i, strerror(errno));
            break;
        }

        usleep(1*1000); // no busy flag, 1ms might be safe
    }

    while(1) {

        // return current date time
        time_t tt;
        struct tm ttm;
        char buf[32];
        tt = time(NULL); // get current time
        localtime_r(&tt, &ttm); // convert to localtime

        strftime(buf, sizeof(buf), "%m/%d %H:%M:%S", &ttm);

        for(i=0; i<strlen(buf); ++i) {
            char cmd[] = { 0x07, 0, 50, 0};
            cmd[1] = i*8;
            cmd[3] = buf[i];
            sz = write(fd, cmd, sizeof(cmd));

            if(sz<0) {
                printf("write hello[%d] failed: %s\n", i, strerror(errno));
                break;
            }

            usleep(1*1000); // no busy flag, 1ms might be safe
        }

        usleep(500*1000);
    }

    close(fd);

    return 0;
}

