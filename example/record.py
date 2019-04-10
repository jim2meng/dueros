#coding:utf-8
import pyaudio
import wave
import json
import signal
import sys
import os

RECORD_RATE = 16000
RECORD_CHANNELS = 2
RECORD_WIDTH = 2
CHUNK = 1024
RECORD_SECONDS = 60
WAVE_OUTPUT_FILENAME = "./output.wav"
#RECORD_DEVICE_NAME = "seeed-2mic-voicecard"
RECORD_DEVICE_NAME = "USB Camera-B4.09.24.1"

p = pyaudio.PyAudio()
stream = p.open(
            rate=RECORD_RATE,
            format=p.get_format_from_width(RECORD_WIDTH),
            channels=RECORD_CHANNELS,
            input=True,
            start=False)

wave_file = wave.open(WAVE_OUTPUT_FILENAME, "wb")

def record():
    wave_file.setnchannels(RECORD_CHANNELS)
    wave_file.setsampwidth(2)
    wave_file.setframerate(RECORD_RATE)
    stream.start_stream()
    print("* recording")
    for i in range(0, int(RECORD_RATE / CHUNK * RECORD_SECONDS)):
        data = stream.read(CHUNK)
        wave_file.writeframes(data)
        
    print("* done recording")
    stream.stop_stream()
    wave_file.close()
    # audio_data should be raw_data
    return("record end")

def sigint_handler(signum, frame):
    stream.stop_stream()
    stream.close()
    p.terminate()
    wave_file.close()
    print 'catched interrupt signal!'
    sys.exit(0)

# 注册ctrl-c中断
signal.signal(signal.SIGINT, sigint_handler)

print p.get_device_count()

device_index=-1

for index in range(0,p.get_device_count()):
    info=p.get_device_info_by_index(index)
    device_name = info.get("name")
    print device_name
    print "\n"
    if device_name.find(RECORD_DEVICE_NAME) != -1:
        device_index=index
        break

if device_index != -1:
    print "find the device"
    stream.close()
    stream = p.open(
            rate=RECORD_RATE,
            format=p.get_format_from_width(RECORD_WIDTH),
            channels=RECORD_CHANNELS,
            input=True,
            input_device_index = device_index,
            start=False)
else:
    print "don't find the device"
    
record()

