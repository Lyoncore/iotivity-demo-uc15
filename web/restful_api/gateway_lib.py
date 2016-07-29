import socket
import math
import decimal
import time
from bottle import route, request, run

print 'create socket'
clientsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

print 'Connecting to local gateway ...'
clientsock.connect(("127.0.0.1", 6655))
print 'Connected'


def read_data(data):
    data = data.ljust(64)
    #print data
    clientsock.send(data)

    value = clientsock.recv(64)
    #print value
    if not value:
        print 'connection closed'
        return None
    else:
        status = value.lstrip().rstrip()
        #print status
        return status

def send_data(name, value):
    name = name.ljust(32)
    value = value.ljust(32)
    name = name + value
    #print name
    clientsock.send(name)



@route('/raspberrypi2/')
def list_device_p():
    return "For raspberrypi2 control"

@route('/raspberrypi2/temperature', method='GET')
def read_temp_p():
    name = 'sensor_p_temp'
    return read_data(name)

@route('/raspberrypi2/humidity', method='GET')
def read_humidity_p():
    name = 'sensor_p_humidity'
    return read_data(name)

@route('/raspberrypi2/light', method='GET')
def read_light_p():
    name = 'sensor_p_light'
    return read_data(name)

@route('/raspberrypi2/sound', method='GET')
def read_sound_p():
    name = 'sensor_p_sound'
    return read_data(name)

@route('/raspberrypi2/led_red', method='GET')
def read_led_p_red():
    name = 'led_p_red'
    value = request.query.value
    if not value:
        return read_data(name)
    else:
        send_data(name, value)

@route('/raspberrypi2/led_green', method='GET')
def read_led_p_green():
    name = 'led_p_green'
    value = request.query.value
    if not value:
        return read_data(name)
    else:
        send_data(name, value)

@route('/raspberrypi2/led_blue', method='GET')
def read_led_p_blue():
    name = 'led_p_blue'
    value = request.query.value
    if not value:
        return read_data(name)
    else:
        send_data(name, value)

@route('/raspberrypi2/lcd', method='GET')
def read_lcd_p():
    name = 'lcd_p'
    value = request.query.value
    if not value:
        return read_data(name)
    else:
        send_data(name, value)

@route('/raspberrypi2/buzzer', method='GET')
def write_buzzer_p():
    name = 'button_p'
    value = request.query.value
    if value:
        send_data(name, value)

@route('/raspberrypi2/button', method='GET')
def read_button_p():
    name = 'button_p'
    return read_data(name)

@route('/raspberrypi2/ultrasonic', method='GET')
def read_ultrasonic_p():
    name = 'sensor_p_ultrasonic'
    return read_data(name)




@route('/arduino/')
def list_device_a():
    return "For arduino control"

@route('/arduino/led', method='GET')
def read_led_a():
    name = 'led_a'
    value = request.query.value
    if not value:
        #print 'read'
        return read_data(name)
    else:
        #print 'write'
        send_data(name, value)

@route('/arduino/lcd', method='GET')
def read_lcd_a():
    name = 'lcd_a'
    value = request.query.value
    if not value:
        #print 'read'
        return read_data(name)
    else:
        #print 'write'
        send_data(name, value)

@route('/arduino/buzzer', method='GET')
def read_buzzer_a():
    name = 'buzzer_a'
    value = request.query.value
    if value:
        send_data(name, value)

@route('/arduino/button', method='GET')
def read_button_a():
    name = 'button_a'
    return read_data(name)

@route('/arduino/touch', method='GET')
def read_touch_a():
    name = 'touch_a'
    return read_data(name)



run(host='localhost', port=5566, debug=True)
