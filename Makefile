
CROSS_COMPILE=arm-linux-gnueabihf-
CC=$(CROSS_COMPILE)gcc
CFLAGS=-I/opt/gpio-lib/WiringBPi_Beta_V2.0/wiringPi -Wall
LDFLAGS=-lwiringPi

default:: gpio lcd spidev_test random-lcd

clean::
	rm -f gpio lcd spidev_test random-lcd

gpio:: gpio.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

lcd:: lcd.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

random-lcd:: random-lcd.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

spidev_test:: spidev_test.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

