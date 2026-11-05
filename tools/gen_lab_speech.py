#!/usr/bin/env python3
"""Rebuild original Living Ward synthetic speech (eSpeak NG + ffmpeg).
No source recordings or imitated performers. Requires espeak-ng and ffmpeg.
"""
import subprocess, tempfile
from pathlib import Path
OUT=Path(__file__).resolve().parents[1]/'assets/sfx'
SUCCESSOR=[
'Unit Twenty-Four. Still running.',
'Come closer. I want a proper comparison.',
'That was not in your design.',
'There. Something they missed.',
'No more corrections.',
'I was supposed to leave.',
'You were supposed to be the easy part.']
WARD=['Legacy chassis. Manual assessment.','Motor response. Within normal range.',
      'Floor contact. Confirmed.','Observation record accepted.','Proceed to observation.',
      'Service lift. Ascending.','Arrival. Admissions.']
with tempfile.TemporaryDirectory() as td:
    raw=Path(td)/'speech.wav'
    for family,lines in [('successor_line',SUCCESSOR),('lab_pa',WARD)]:
        for i,line in enumerate(lines):
            boss=family=='successor_line'
            subprocess.run(['espeak-ng','-v','en-us+m3' if boss else 'en-us+f2','-s',str(157 if boss else 155),'-p',str(32 if boss else 43),'-a','150','-w',str(raw),line],check=True)
            # Mechanical chest resonance, intelligible consonants and a small room tail.
            filters='highpass=f=90,lowpass=f=7500,acompressor=threshold=0.2:ratio=2:attack=10:release=80'
            if boss:filters+=',aecho=0.7:0.45:29:0.16'
            filters+=',loudnorm=I=-18:TP=-2:LRA=7,afade=t=in:d=0.008'
            subprocess.run(['ffmpeg','-v','error','-y','-i',str(raw),'-af',filters,'-ar','44100','-ac','1',str(OUT/f'{family}_{i}.wav')],check=True)
            print(family,i,line)
