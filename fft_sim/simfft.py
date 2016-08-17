#!/usr/bin/python

import sys, serial, argparse
import numpy as np
import glob
from collections import deque
from time import sleep

import matplotlib.animation as animation
import matplotlib.pyplot as plt 
import timeit

maxLen = 1000

Fs = 100
x = np.arange(maxLen)

Amp_t = 1.0
f_t = 5
w_s = 2*np.pi*f_t

phase = np.pi / 2

s0 = np.sin( w_s * x / Fs)

Amp_r = 1.0
f_d = 0.001
w_d = 2*np.pi*f_d
a = [0]*maxLen
#for i in range(analogPlot.maxLen):
#        a[i]=np.sin( (w_s + w_d) * i/Fs + phase)
s1 = np.sin( (w_s + w_d) * x / Fs + phase)
#analogPlot.s1 = a

#s2 = s1 * s0

#s0 = np.sin( w_s * x / Fs)
#s1 = np.sin( (w_s + w_d) * x / Fs + phase)
#s2 = s1 * s0

import time

t0 = time.time()

for i in range(100):
    freq_list = np.fft.rfft(s0)

t1 = time.time()
total = t1-t0
print total

t0 = time.time()

for i in range(100):
    freq_list = np.fft.rfft(s0)

t1 = time.time()
total = t1-t0
print total

plt.plot(freq_list)

#plt.plot(x,s0)
plt.show()


