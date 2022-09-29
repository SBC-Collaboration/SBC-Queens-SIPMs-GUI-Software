import numpy as np
import serial as ser
import time

def send_gpib(port, msg):
    port.write((msg + '\n').encode('ascii'))
    time.sleep(0.100)

def send_kei2000(port, msg):
    send_gpib(port, '++addr 10')
    send_gpib(port, msg)

def send_kei6487(port, msg):
    send_gpib(port, '++addr 22')
    send_gpib(port, msg)

with ser.Serial('/dev/ttyUSB0', timeout = 1000) as port:
    send_gpib(port, '++eos 0')
    send_gpib(port, '++auto 0')

    #  Keitheley 2000
    send_gpib(port, '++addr 10')
    send_gpib(port, '*rst')
    send_gpib(port, ':init:cont on')
    send_gpib(port, ':volt:dc:nplc 10')
    send_gpib(port, ':volt:dc:rang:auto 0')
    send_gpib(port, ':volt:dc:rang 60')
    send_gpib(port, ':volt:dc:aver:stat 1')
    send_gpib(port, ':volt:dc:aver:tcon mov')
    send_gpib(port, ':volt:dc:aver:coun 100')

    send_gpib(port, ':form ascii')

    #  Keithley 6487
    send_gpib(port, '++addr 22')
    send_gpib(port, '*rst')
    send_gpib(port, ':sour:volt:rang 60')
    send_gpib(port, ':sour:volt:ilim 25e-6')
    send_gpib(port, ':sour:volt 0.0V')
    send_gpib(port, ':sour:volt:stat ON')

    send_gpib(port, '++auto 1')

    v_init = 0.0
    d_v = 1.0
    v_final = 60.0
    n_samples = 10
    out = np.zeros(( n_samples*int((v_final - v_init) / d_v), 2))
    for i, V in enumerate(np.arange(v_init, v_final, d_v)):

        print(':sour:volt ' + str(V))
        send_kei6487(port, ':sour:volt ' + str(V))
        time.sleep(60*10)

        for n in range(n_samples):
            send_kei2000(port, ':fetch?')
            value = port.readline()

            print(value)

            out[n_samples*i + n, 0] = V
            out[n_samples*i + n, 1] = float(value.decode('ASCII'))

            time.sleep(60)

    np.savetxt("keithley2000_cal.csv", out, delimiter=",")

    send_gpib(port, 'sour:stat OFF')