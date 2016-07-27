#!/usr/bin/python

import sys, serial, argparse
import numpy as np
import glob
from collections import deque
from time import sleep

import matplotlib.animation as animation
import matplotlib.pyplot as plt 

	
class AnalogPlot:
	def __init__(self, maxLen):

		self.s0 = deque([0.0]*maxLen)
		self.s1 = deque([0.0]*maxLen)
		self.s2 = deque([0.0]*maxLen)
		self.maxLen = maxLen


                self.Fs = 8000
                self.x = np.arange(self.maxLen)

                self.Amp_t = 1.0
                self.f_t = 5
                self.w_s = 2*np.pi*self.f_t

                self.phase = np.pi / 2

                self.s0 = np.sin( self.w_s * self.x / self.Fs)


                self.Amp_r = 1.0
                self.f_d = 0.001
                self.w_d = 2*np.pi*self.f_d
                self.a = [0]*self.maxLen
                #for i in range(analogPlot.maxLen):
                #        a[i]=np.sin( (w_s + w_d) * i/Fs + phase)
                self.s1 = np.sin( (self.w_s + self.w_d) * self.x / self.Fs + self.phase)
                #self.analogPlot.s1 = a

                self.s2 = self.s1 * self.s0

	# add to buffer
	def addToBuf(self, buf, val):
		if len(buf) < self.maxLen:
			buf.append(val)
		else:
			buf.pop()
			buf.appendleft(val)

	# update plot
	def update(self, frameNum, a0, a1, a2):

                self.calc_waveforms()

                a0.set_data(range(self.maxLen), self.s0)
                a1.set_data(range(self.maxLen), self.s1)
                a2.set_data(range(self.maxLen), self.s2)

        def calc_waveforms(self):
            self.s0 = np.sin( self.w_s * self.x / self.Fs)
            self.s1 = np.sin( (self.w_s + self.w_d) * self.x / self.Fs + self.phase)
            self.s2 = self.s1 * self.s0



def press(event, a0, a1, a2, analogPlot):
    sys.stdout.flush()
    if event.key == '1':
        if a0.get_linestyle() == 'None':
            a0.set_linestyle('-')
            print 'sensor', event.key, 'on'
        else:
            a0.set_linestyle('None')
            print 'sensor', event.key, 'off'
    if event.key == '2':
        if a1.get_linestyle() == 'None':
            a1.set_linestyle('-')
            print 'sensor', event.key, 'on'
        else:
            a1.set_linestyle('None')
            print 'sensor', event.key, 'off'
    if event.key == '3':
        if a2.get_linestyle() == 'None':
            a2.set_linestyle('-')
            print 'sensor', event.key, 'on'
        else:
            a2.set_linestyle('None')
            print 'sensor', event.key, 'off'
    if event.key == '-':
        analogPlot.phase += np.pi / 100

def main():
        analogPlot = AnalogPlot(8000)

	fig = plt.figure()
	ax = plt.axes(xlim=(0, analogPlot.maxLen), ylim=(-10, 10))
	a0, = ax.plot([], [])
	a1, = ax.plot([], [])
	a2, = ax.plot([], [])

        fig.canvas.mpl_connect('key_press_event', lambda event: press(event, a0, a1, a2,analogPlot))

	anim = animation.FuncAnimation(fig, analogPlot.update, 
								 fargs=(a0,a1,a2,), 
								 interval=1)
	plt.show()
	analogPlot.close()

	print('exiting.')

if __name__ == '__main__':
	main()
