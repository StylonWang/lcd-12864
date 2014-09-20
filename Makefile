
CROSS_COMPILE=
CC=$(CROSS_COMPILE)gcc

default:: gpio lcd 

gpio:: gpio.c
	$(CC) -I/opt/gpio-lib/WiringBPi_Beta_V2.0/wiringPi $^ -lwiringPi -o $@

lcd:: lcd.c
	$(CC) -I/opt/gpio-lib/WiringBPi_Beta_V2.0/wiringPi $^ -lwiringPi -o $@


