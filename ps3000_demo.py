"""
PS3000a Demo.

By: Colin O'Flynn, based on Mark Harfouche's software

This is a demo of how to use AWG with the Picoscope 2204 along with capture
It was tested with the PS2204A USB2.0 version

The AWG is connected to Channel A.
Nothing else is required.

NOTE: Must change line below to use with "A" and "B" series PS3000a models

"""
from __future__ import division
from __future__ import absolute_import
from __future__ import print_function
from __future__ import unicode_literals

#import time
from picoscope import ps3000a
import pylab as plt
import numpy as np
import matplotlib.animation as animation
from scipy import signal

def butter_highpass(cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = signal.butter(order, normal_cutoff, btype='high', analog=False)
    return b, a

def butter_lowpass(cutoff, fs, order=5):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = signal.butter(order, normal_cutoff, btype='low', analog=False)
    return b, a

def butter_highpass_filter(data, cutoff, fs, order=5):
    b, a = butter_highpass(cutoff, fs, order=order)
    y = signal.filtfilt(b, a, data)
    return y

def butter_lowpass_filter(data, cutoff, fs, order=5):
    b, a = butter_lowpass(cutoff, fs, order=order)
    y = signal.filtfilt(b, a, data)
    return y

def update(frame):
    global scopeRunning

    if scopeRunning:
        if ps.isReady():
            #print("Done waiting for trigger")
            dataA = ps.getDataV('A', nSamples, returnOverflow=False)
            dataB = ps.getDataV('B', nSamples, returnOverflow=False)
            filtered_B = butter_highpass_filter(dataB,50000,Fs)
            shifted_B = np.zeros(len(filtered_B))
            shifted_B[:-1500] = filtered_B[1500:]
            #filtered_B = dataC
            #dataM = dataA*dataB
            dataM = dataA*shifted_B


            #dataM1 = butter_lowpass_filter(dataM, 1.2E6, Fs)
            dataM1 = butter_lowpass_filter(dataM, 1.2E5, Fs)
            line1.set_ydata(dataA)
            line2.set_ydata(dataB)
            line3.set_ydata(shifted_B)
            line4.set_ydata(dataM)
            line5.set_ydata(dataM1)
            scopeRunning = False
        else:
            return
            #print("Waiting for trigger")



    if not scopeRunning:
        ps.runBlock()
        scopeRunning = True
        #print("Set up trigger")

    #ps.runBlock()
    #ps.waitReady()
    #fig.canvas.draw()
    #plt.pause(1)
    #    line1.set_ydata(np.sin(
    #return line1


if __name__ == "__main__":
    print(__doc__)

    scopeRunning = False

    ps = ps3000a.PS3000a()

    #print(ps.getAllUnitInfo())
    print("Opened PicoScope successfully")

    #waveform_desired_duration = 50E-6
    waveform_desired_duration = 8E-6

    #obs_duration = 1 * waveform_desired_duration
    obs_duration = 10 * waveform_desired_duration
    #sampling_interval = obs_duration / 4096
    #sampling_interval = obs_duration / 1000
    Fs = 50E6
    sampling_interval = 1.0 / Fs

    (actualSamplingInterval, nSamples, maxSamples) = \
        ps.setSamplingInterval(sampling_interval, obs_duration)

    Fs = 1.0 / actualSamplingInterval
    print("Sampling interval = %f ns" % (sampling_interval * 1E9))
    print("Actual sampling interval = %f ns" % (actualSamplingInterval * 1E9))
    print("Sampling frequency = %f MHz" % (Fs * 1E-6))
    print("Taking  samples = %d" % nSamples)
    print("Maximum samples = %d" % maxSamples)
    print("Window length = %f us" % (nSamples*actualSamplingInterval * 1E6))

    # the setChannel command will chose the next largest amplitude
    # original signal
    channelRange = ps.setChannel('A', 'DC', 20.0, 0.0, enabled=True, BWLimited=False)
    #print("Chosen channel range = %d" % channelRange)
    # returned echo
    channelRange = ps.setChannel('B', 'DC', 0.1, 0.0, enabled=True, BWLimited=False)

    #ps.setSimpleTrigger('A', 1.0, 'Falling', timeout_ms=100, enabled=True)
    ps.setSimpleTrigger('External', 0.5, 'Rising', timeout_ms=200, enabled=True)

    ps.runBlock()
    ps.waitReady()
    print("Done waiting for trigger")
    dataA = ps.getDataV('A', nSamples, returnOverflow=False)
    dataB = ps.getDataV('B', nSamples, returnOverflow=False)
    dataM = dataA*dataB
    dataM1 = dataA*dataB

    dataTimeaxis = np.arange(nSamples) * actualSamplingInterval

    #ps.stop()
    #ps.close()

    #Uncomment following for call to .show() to not block
    #plt.ion()

    #fig = plt.figure()
    fig, axes = plt.subplots(5, sharex=True, figsize=(20,12))
    #fig.set_size_inches(20.5, 20.5)
    ax_t = axes[0]
    ax_r = axes[1]
    ax_rf = axes[2]
    ax_m = axes[3]
    ax_m1 = axes[4]
    #ax_t = fig.add_subplot(111)
    #ax_r = fig.add_subplot(111)
    line1, = ax_t.plot(dataTimeaxis, dataA, label="Clock")
    line2, = ax_r.plot(dataTimeaxis, dataB, label="Clock")
    line3, = ax_rf.plot(dataTimeaxis, dataB, label="Clock")
    line4, = ax_m.plot(dataTimeaxis, dataM, label="Clock")
    line5, = ax_m.plot(dataTimeaxis, dataM1, label="Clock")
    ax_t.set_ylim([-11,11])
    ax_r.set_ylim([-0.1,0.1])
    ax_rf.set_ylim([-0.1,0.1])
    ax_m.set_ylim([-1,1])
    ax_m1.set_ylim([-1,1])

    #plt.hold(True)
    #plt.plot(dataTimeaxis, dataA, label="Clock")
    #plt.grid(True, which='major')
    plt.title("Picoscope 3000a doppler")
    plt.ylabel("Voltage (V)")
    plt.xlabel("Time (ms)")
    plt.legend()

    ani = animation.FuncAnimation(fig, update, blit=False, interval=5)
    plt.show()

