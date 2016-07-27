import SocketServer
import serial
import getopt
import os
import glob
import socket
import sys
import time
import datetime
from struct import *
from collections import deque
import matplotlib.animation as animation
import matplotlib.pyplot as plt 
from scipy.signal import butter, lfilter, freqz

#DATA_LEN = 10000
DATA_LEN = 20000

MixedVoltages = deque(maxlen=DATA_LEN)

def update_graph(frameNum, a0):
    a0.set_data(range(DATA_LEN),MixedVoltages)
    plt.draw()
    plt.pause(0.1)
    print "update graph"

counter = 0

def butter_lowpass(cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    return b, a

def butter_lowpass_filter(data, cutoff, fs, order=5):
    b, a = butter_lowpass(cutoff, fs, order=order)
    y = lfilter(b, a, data)
    return y

class MyUDPHandler(SocketServer.BaseRequestHandler):

    def handle(self):
        data = self.request[0].strip()
        socket = self.request[1]
        #print "{} wrote:".format(self.client_address[0])
        #channel, Vmax, Vmin = unpack('cii', data)
        if len(data) != 4:
            #print "data length: " , len(data)
            return
        V, = unpack('i', data)
        #print V

        global counter
        global capture
        counter += 1
        #capture = False

        if counter%1000 == 0:
            #print "counter: ", counter
            if capture:

                order = 6
                fs = 100000.0
                cutoff = 100.0
                #cutoff = 200.0
                
                a0.set_data(range(DATA_LEN),butter_lowpass_filter(MixedVoltages, cutoff, fs, order))
                #a0.set_data(range(DATA_LEN),MixedVoltages)
                plt.draw()
                #plt.pause(0.001)

        MixedVoltages.popleft()
        MixedVoltages.append(V)


        #if channel == 'A':
        #    AVoltages.popleft()
        #    AVoltages.append(Vmax)
        #if channel == 'B':
        #    BVoltages.popleft()
        #    BVoltages.append(Vmax)
        #if channel == "X":
        #    #self.server_close()
        #    #self.shutdown()
        #    #self.interrupt_main()
        #    raise KeyboardInterrupt
               
        #socket.sendto(data.upper(), self.client_address)

def press(event, capture):
    print "capture event"
    sys.stdout.flush()
    if event.key == ' ':
        capture = not capture
        print "toggle capture"

if __name__ == "__main__":

    # Initialise queues

    for i in range(0,DATA_LEN):
        MixedVoltages.append(0)

    # Initialise plot
    fig = plt.figure()
    ax = plt.axes(xlim=(0, DATA_LEN), ylim=(-5000, 5000))
    plt.ion()
    plt.show()
    a0, = ax.plot([], [])

    capture = True

    fig.canvas.mpl_connect('key_press_event', lambda event: press(event, capture))

    #anim = animation.FuncAnimation(fig, update_graph, 
    #                             fargs=(a0,a1,a2,), 
    #                             interval=1)


    HOST, PORT = "127.0.0.1", 45454
    server = SocketServer.UDPServer((HOST, PORT), MyUDPHandler)
    print "Server ready"
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        server.shutdown()


