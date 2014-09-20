#!/usr/bin/env python

import wiringpi
#2 as wiringpi

print sys.argv[0]
exit

# wiringpi numbers

wiringpi.wiringPiSetup()

wiringpi.pinMode(6, 1) # sets WP pin 6 to output

wriingpi.digitalWrite(6, 1)

