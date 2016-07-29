#!/usr/bin/env python

import math
import decimal
import time
import grovepi
from grove_rgb_lcd import grove_rgb_lcd

# LED configs
led_red = 4
led_green = 5
led_blue = 6

# DHT11 sensor
dht11_sensor_port = 7     # Connect the DHt sensor to port D7

# Connect the Grove Light Sensor to analog port A0
light_sensor = 0

# Connect the Grove Sound Sensor to analog port A1
# SIG,NC,VCC,GND
sound_sensor = 1

# Connect the Grove Buzzer to digital port D8
# SIG,NC,VCC,GND
buzzer = 8

# Connect the Grove Button to digital port D3
# SIG,NC,VCC,GND
button = 3

# Connect the Grove Ultrasonic Ranger to digital port D2
# SIG,NC,VCC,GND
ultrasonic_ranger = 2




def pin_config():
        grovepi.pinMode(light_sensor,"INPUT")
	grovepi.pinMode(sound_sensor,"INPUT")
        grovepi.pinMode(led_red,"OUTPUT")
        grovepi.pinMode(led_green,"OUTPUT")
        grovepi.pinMode(led_blue,"OUTPUT")
	grovepi.pinMode(buzzer,"OUTPUT")
        grovepi.pinMode(button,"INPUT")

def CtoF( tempc ):
        "This converts celcius to fahrenheit"
        tempf = round((tempc * 1.8) + 32, 2);
        return tempf;

def FtoC( tempf ):
        "This converts fahrenheit to celcius"
        tempc = round((tempf - 32) / 1.8, 2)
        return tempc;

def sensor_read_temp():
        temp = 0.01
        hum = 0.01
	retry = 3
	while retry > 0:
		try:
        		[ temp,hum ] = grovepi.dht(dht11_sensor_port,0)
        		time.sleep(.1)
			if math.isnan(temp):
				print 'fail to read temperature sensor, retry again'
				retry = retry -1
			else:
				break
		except:
			print 'can not read temperature sensor, retry again'
			retry = retry - 1
        		time.sleep(.1)
	if retry == 0:
		return -1
	else:
        	return temp
                
def sensor_read_humidity():
        temp = 0.01
        hum = 0.01
	retry = 3
	while retry > 0:
		try:
        		[ temp,hum ] = grovepi.dht(dht11_sensor_port,0)
        		time.sleep(.1)
			if math.isnan(hum):
				print 'fail to read humidity sensor, retry again'
				retry = retry - 1
			else:
				break
		except:
			print 'can not read humidity sensor, retry again'
			retry = retry - 1
        		time.sleep(.1)
	if retry == 0:
		return -1
	else:
        	return hum

def sensor_read_light():
	retry = 3
	while retry > 0:
		try:
        		sensor_value = grovepi.analogRead(light_sensor)
        		time.sleep(.1)
			break;
		except:
			print 'can not read light sensor, retry again'
			retry = retry - 1
        		time.sleep(.1)

	if retry == 0:
		return -1
	else:
        	return sensor_value

def sensor_read_sound():
	retry = 3
	while retry > 0:
		try:
        		sensor_value = grovepi.analogRead(sound_sensor)
        		time.sleep(.1)
			break
		except:
			print 'can not read sound sensor, retry again'
			retry = retry - 1
        		time.sleep(.1)

	if retry == 0:
		return -1
	else:	
        	return sensor_value

def led_write_red(value):
	retry = 3
	while retry > 0:
		try:
        		time.sleep(.2)
        		grovepi.digitalWrite(led_red, value)
			break
		except:
			print 'can not write red led, retry again'
			retry = retry - 1
        		time.sleep(.2)
			
	if retry == 0:
		return -1
	else:
		return 0

def led_write_green(value):
	retry = 3
	while retry > 0:
		try:
        		time.sleep(.2)
        		grovepi.digitalWrite(led_green, value)
			break
		except:
			print 'can not write green led, retry again'
			retry = retry - 1
        		time.sleep(.2)
			
	if retry == 0:
		return -1
	else:
		return 0

def led_write_blue(value):
	retry = 3
	while retry > 0:
		try:
        		time.sleep(.2)
        		grovepi.digitalWrite(led_blue, value)
			break
		except:
			print 'can not write blue led, retry again'
			retry = retry - 1
        		time.sleep(.2)
			
	if retry == 0:
		return -1
	else:
		return 0

def lcd_write_str(value):
	#print "Write string: "
	#print value
	retry = 3
	while retry > 0:
		try:
   		    	time.sleep(.2)
			grove_rgb_lcd.setRGB(0, 255, 0)
			grove_rgb_lcd.setText(value)
			break
		except:
			print 'can not write lcd, retry again'
			retry = retry - 1
        		time.sleep(.2)
			
	if retry == 0:
		return -1
	else:
		return 0

def buzzer_write(value):
        grovepi.digitalWrite(buzzer,1)
        time.sleep(value)
        grovepi.digitalWrite(buzzer,0)
        time.sleep(.2)

def button_read():
	retry = 3
	while retry > 0:
		try:
        		status = grovepi.digitalRead(button)
		        time.sleep(.2)
			break
		except:
			print 'can not read button, retry again'
			retry = retry - 1
        		time.sleep(.2)
		
	if retry == 0:
		return -1
	else:
        	return status

def ultrasonic_read():
	retry = 3
	while retry > 0:
		try:
			status = grovepi.ultrasonicRead(ultrasonic_ranger)
			time.sleep(.2)
			break
		except:
			print 'can not read ultrasonic, retry again'
			retry = retry - 1
			time.sleep(.2)

	if retry == 0:
		return -1
	else:
		return status

