
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
#include <sys/time.h>

unsigned char output_buf[5 + 128*(64/8)]; // cmd: 1byte, x-y-w-h: 4 bytes
unsigned char display_buf[64][128];

void random_draw_lcd(void)
{
    int x, y;

    memset(display_buf, 0, sizeof(display_buf));

    for(y=0; y<64; ++y) {
        for(x=0; x<128; ++x) {
            display_buf[y][x] = rand()%2;
        }
    }
}

void horizontal_line_draw_lcd(void)
{
    int x, y;

    memset(display_buf, 0, sizeof(display_buf));

    for(y=0; y<64; y+=2) {
        for(x=0; x<128; x++) {
            display_buf[y][x] = 1;
        }
    }
}

void vertical_line_draw_lcd(void)
{
    int x, y;

    memset(display_buf, 0, sizeof(display_buf));

    for(y=0; y<64; y++) {
        for(x=0; x<128; x+=2) {
            display_buf[y][x] = 1;
        }
    }
}

// MzLH04 has an internal buffer of 400 bytes but no busy flag.
// Break up the writes and put some delay.
ssize_t buffer_write(int fd, const void *buf, size_t count)
{
    ssize_t total = 0;
    int limit = 40;

    while(total<count) {

        if(count-total>limit) {
            ssize_t sz = write(fd, buf+total, limit);
            //printf("bwrite %ld\n", (long)sz);
            if(sz<0) return sz;
            else total += sz;
        }
        else {
            ssize_t sz = write(fd, buf+total, count-total);
            //printf("bwrite %ld\n", (long)sz);
            if(sz<0) return sz;
            else total += sz;
        }

        usleep(3*1000);
    }

    return total;
}

void display_lcd(int fd)
{
    int x, y, k;
    ssize_t sz;

    output_buf[0] = 0x0E; // show bit map
    output_buf[1] = 0; // X-coordinate: 0
    output_buf[2] = 0; // Y-coordinate: 0
    output_buf[3] = 128; // 128 pixels in width
    output_buf[4] = 64; // 64 pixels in height

    memset(output_buf+5, 0 ,sizeof(output_buf)-5);
    // TODO: transform display_buf to output_buf in bitmap
    for(y=0; y<64; ++y) {
        for(x=0; x<(128/8); ++x) {
            unsigned char *p = &display_buf[y][x*8];

            for(k=0; k<8; ++k) {
                output_buf[5+y*(128/8)+x] += (p[k] << (7-k));
            }
        }
    }

    sz = buffer_write(fd, output_buf, sizeof(output_buf));
    if(sz!=sizeof(output_buf)) {
        printf("graphic write %ld instead of %d bytes\n", (long)sz, sizeof(output_buf));
    }

    //usleep(1*1000);
}


int main(int argc, char **argv)
{
    int ret;
    int fd;

    char clr[] = {0x80};
    char backlight[] = {0x8A, 30};

    #define CMD(x) { x, sizeof(x) }
    struct { char *cmd; int len; } 
        cmd[] = {
                    CMD(clr),
                    CMD(backlight),
                };

    ssize_t sz;
    uint8_t mode = SPI_CPHA|SPI_CPOL; // clock high active, latch on rising edge
    uint8_t bits = 8;
    uint32_t speed = 500000;
    int i;
    int count=0;
    struct timeval tv1, tv2;

    srand(time(NULL));

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

    gettimeofday(&tv1, NULL);

    while(1) {
        count++;

        switch(count%3) {

        case 0:
            random_draw_lcd();
            display_lcd(fd);
            break;

        case 1:
            horizontal_line_draw_lcd();
            display_lcd(fd);
            break;

        case 2:
            vertical_line_draw_lcd();
            display_lcd(fd);
            break;

        }

        // get FPS
        if(count>100) {
            unsigned long milisec;

            gettimeofday(&tv2, NULL);
            milisec = (tv2.tv_sec-tv1.tv_sec)*1000;
            if(tv2.tv_usec>tv1.tv_usec) {
                milisec += (tv2.tv_usec-tv1.tv_usec)/1000;
            }
            else {
                milisec -= 1000;
                milisec += (tv2.tv_usec/1000)+1000 - (tv1.tv_usec)/1000;
            }
            printf("FPS: %2.2f\n", (float)count/(milisec+1)*1000);

            count = 0;
            gettimeofday(&tv1, NULL);
        }
    }

    close(fd);

    return 0;
}

