#!/usr/bin/env python3
"""Original Shepherd cue vocabulary and a labelled audition reel. Stdlib only.

Reuses the frozen CC0 clank texture and DSP helpers from gen_combat_audio;
never reads a generated output as source. No commercial audio is used.
"""
import math
from pathlib import Path
import random
import struct
import wave
from gen_combat_audio import SR, TAU, band, body, lowpass, metal, recording, save

# name: (duration, MixTable target). Three separately excited variants each.
CUES = {
    "shep_fist_tell": (.52, -14), "shep_sweep_tell": (.55, -14),
    "shep_grab_tell": (.52, -14), "shep_gaze_tell": (.58, -14),
    "shep_lock": (.12, -14), "shep_parry": (.13, -14),
    "shep_memory": (.21, -14), "shep_moon": (.23, -13),
    "shep_gather": (.82, -14), "shep_contact": (.72, -10),
    "shep_sweep": (.43, -10), "shep_grip": (.34, -10),
    "shep_pin": (.31, -10), "shep_strain": (.90, -13),
    "shep_counter": (.88, -10), "shep_break": (1.20, -10),
    "shep_seal": (.35, -10), "shep_implode": (1.65, -10),
}


def make(name, variant, rec):
    duration, _ = CUES[name]
    rng = random.Random(9617 + variant * 997 + sum((k+1)*ord(c) for k,c in enumerate(name)))
    n = int(duration * SR)
    noise = [rng.uniform(-1, 1) for _ in range(n)]
    edge = band(noise, 650, 5100)
    air = band(noise, 220, 2100)
    phases = [rng.uniform(-.2, .2) for _ in range(5)]
    pitch = 1 + (variant-1)*.025
    out = []
    for i in range(n):
        t = i / SR
        tt = t * pitch
        grain = edge[i]*math.exp(-t*90)
        v = 0.
        if name == "shep_fist_tell":
            for start, freq in ((0, 310), (.17, 465)):
                u = tt-start
                if u >= 0: v += metal(u, freq, 16, phases)*.5 + body(u, 210, 95, 24)*.35
            v += math.sin(TAU*(720*tt+250*tt*tt))*math.exp(-t*8)*.12
        elif name == "shep_sweep_tell":
            v = air[i]*(.5+1.7*t)*math.exp(-t*5) + metal(tt, 520, 9, phases)*.28
            v += math.sin(TAU*(360*tt+1050*tt*tt))*math.exp(-t*6)*.2
        elif name == "shep_grab_tell":
            for start in (0, .085, .17, .255):
                u = tt-start
                if u >= 0: v += metal(u, 810-start*900, 48, phases)*.33
            v += math.sin(TAU*(460*tt-180*tt*tt))*math.exp(-t*8)*.28
        elif name == "shep_gaze_tell":
            v = (math.sin(TAU*(810*tt+350*tt*tt))+.5*math.sin(TAU*(1145*tt+500*tt*tt)))*math.exp(-t*6)*.32
            v += grain*.2
        elif name in ("shep_lock", "shep_parry", "shep_memory", "shep_moon"):
            freq = {"shep_lock":1100,"shep_parry":2100,"shep_memory":570,"shep_moon":1420}[name]
            v = metal(tt, freq, 24 if name=="shep_memory" else 36, phases)*.7 + grain*.35
            v += body(tt, 280, 115, 40)*(.5 if name=="shep_memory" else .2)
        elif name == "shep_gather":
            v = (math.sin(TAU*(370*tt-140*tt*tt))+.35*math.sin(TAU*(554*tt-200*tt*tt)))*math.exp(-t*3.7)*.5
            v += air[i]*math.exp(-t*5)*.3 + body(tt, 160, 58, 10)*.35
        elif name == "shep_strain":
            env = .25+.75*(t/duration)**1.4
            v = (math.sin(TAU*(180*tt+230*tt*tt))+.45*math.sin(TAU*(275*tt+350*tt*tt)))*env*.5
            v += air[i]*env*.2
        else:
            freq, decay = {"shep_contact":(185,8),"shep_sweep":(390,12),"shep_grip":(580,20),
                           "shep_pin":(1230,18),"shep_counter":(260,7),"shep_break":(340,5),
                           "shep_seal":(740,18),"shep_implode":(145,3.5)}[name]
            j = int(i*(.8 if name in ("shep_contact","shep_implode") else 1.05)*pitch)
            texture = rec[j] if j < len(rec) else 0.
            v = metal(tt, freq, decay, phases)*.25 + texture*math.exp(-t*3)*.5
            v += body(tt, 230, 52 if name in ("shep_counter","shep_implode") else 78, 12)*.8 + grain*.7
            if name == "shep_sweep": v += air[i]*math.exp(-t*8)*1.1
            if name in ("shep_counter","shep_break","shep_implode"):
                for start in (.055,.13,.23):
                    if t > start: v += edge[i]*math.exp(-(t-start)*35)*.23
            if name == "shep_implode":
                u = t-.22
                if u > 0: v += math.sin(TAU*53*u)*(1-math.exp(-u*30))*math.exp(-u*3)*.45
        v *= min(1., t/.0004)*min(1., (duration-t)/.02)
        out.append(math.tanh(v*1.7))
    dc = lowpass(out, 25)
    return [(v-d)*min(1., (n-1-i)/(SR*.01)) for i,(v,d) in enumerate(zip(out,dc))]


def main():
    rec = recording()
    for name, (_, target) in CUES.items():
        for variant in range(3):
            save(name+(f"_{variant}" if variant else ""), make(name,variant,rec), target)
    root = Path(__file__).resolve().parents[1]
    dest = root/"build"/"shepherd-audio"
    dest.mkdir(parents=True,exist_ok=True)
    pcm = bytearray()
    lines = ["# Shepherd cue reel", "", "Base variant of each cue, separated by 0.6 seconds. Original asset levels; no music.", ""]
    for name, (duration, _) in CUES.items():
        lines.append(f"- {len(pcm)/(SR*2):05.2f}s: {name} ({duration:.2f}s)")
        with wave.open(str(root/"assets"/"sfx"/(name+".wav"))) as w: pcm.extend(w.readframes(w.getnframes()))
        pcm.extend(bytes(int(.6*SR)*2))
    with wave.open(str(dest/"shepherd-cues.wav"),"wb") as w:
        w.setparams((1,2,SR,0,"NONE","not compressed"));w.writeframes(pcm)
    (dest/"README.md").write_text("\n".join(lines)+"\n")


if __name__ == "__main__": main()
