#coding:utf-8
import pyaudio
import wave
import json
import signal
import sys
import os

RECORD_RATE = 16000
RECORD_CHANNELS = 1
RECORD_WIDTH = 2
CHUNK = 1024
RECORD_SECONDS = 60
WAVE_OUTPUT_FILENAME = "./output.wav"

p = pyaudio.PyAudio()
stream = p.open(
            rate=RECORD_RATE,
            format=p.get_format_from_width(RECORD_WIDTH),
            channels=RECORD_CHANNELS,
            input=True,
            start=False)

wave_file = wave.open(WAVE_OUTPUT_FILENAME, "wb")

def record():
    wave_file.setnchannels(1)
    wave_file.setsampwidth(2)
    wave_file.setframerate(16000)
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

record()   

