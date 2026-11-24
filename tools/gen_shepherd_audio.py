#!/usr/bin/env python3
"""Original Chapter 6 cue vocabulary and labelled audition reels. Stdlib only.

Reuses the frozen CC0 clank texture and DSP helpers from gen_combat_audio;
never reads a generated output as source. No commercial audio is used.
"""
import math
import argparse
from pathlib import Path
import random
import struct
import wave
from functools import lru_cache
from gen_combat_audio import SR, TAU, band, body, lowpass, metal, recording, save

# name: (duration, MixTable target). Three separately excited variants each.
CUES = {
    "shep_fist_tell": (.52, -14), "shep_sweep_tell": (.55, -14),
    "shep_grab_tell": (.52, -14), "shep_gaze_tell": (.58, -14),
    "shep_lock": (.12, -14), "shep_parry": (.13, -14),
    "shep_memory": (.21, -14), "shep_moon": (.23, -13),
    "shep_gather": (.82, -14), "shep_contact": (1.45, -10),
    "shep_sweep": (1.15, -10), "shep_grip": (.75, -10),
    "shep_pin": (.60, -10), "shep_strain": (.90, -13),
    "shep_counter": (1.60, -10), "shep_break": (2.10, -10),
    "shep_seal": (.55, -10), "shep_implode": (2.60, -10),
    "shep_beam_tell": (.64, -14), "shep_gaze_fire": (.52, -10),
    "shep_beam": (.85, -10),
    "shep_wound": (.48, -10), "shep_wound_heavy": (1.15, -10),
    "shep_motion": (.32, -18), "shep_lift": (.48, -18),
    "ring_knock": (.48, -18), "ring_creak": (.85, -18),
    "ring_sweep_warn": (.68, -14), "ring_sweep_pass": (.85, -13),
    "ring_debris": (.72, -18),
}

@lru_cache(maxsize=None)
def mechanism_source(name):
    path=Path(__file__).resolve().parents[1]/"assets"/"sfx"/"src"/(name+".wav")
    with wave.open(str(path)) as w:
        assert (w.getnchannels(),w.getsampwidth(),w.getframerate())==(1,2,SR)
        return band([x/32768 for x in struct.unpack("<%dh"%w.getnframes(),w.readframes(w.getnframes()))],150,5400)


# Recorded-steel bite (gain, decay/s): tuned with tools/listen.py so each tell
# has enough 1.5-6 kHz energy to clear the rifle without turning into a chime.
FIST_BITE = (1.7, 22)
GATHER_BITE = (1.8, 15)


@lru_cache(maxsize=None)
def bright_source(name):
    """The 1.8-7.5 kHz band of a real recording: brightness that stays metal.

    Pure-tone pings here read as a chime ("de-do") in the user's audition;
    the recorded band keeps the material's grit and has no melody.
    """
    path=Path(__file__).resolve().parents[1]/"assets"/"sfx"/"src"/(name+".wav")
    with wave.open(str(path)) as w:
        raw=[x/32768 for x in struct.unpack("<%dh"%w.getnframes(),w.readframes(w.getnframes()))]
    start=max(range(len(raw)),key=lambda i:abs(raw[i]))   # read from the strike itself
    hi=band(raw[max(0,start-int(.002*SR)):],1800,7500)
    peak=max(abs(x) for x in hi) or 1.
    return [x/peak for x in hi]


def make(name, variant, rec):
    """Discrete mechanisms and resonances, with silence between their gestures.

    Attack families differ in rhythm, register and recorded material. Idle has
    no sustained noise, swells or bed; attacks do not layer generic movement.
    Noise is reserved for contact transients and the final implosion/debris.
    """
    duration, _ = CUES[name]
    rng = random.Random(9617 + variant * 997 + sum((k+1)*ord(c) for k,c in enumerate(name)))
    n = int(duration * SR)
    noise = [rng.uniform(-1, 1) for _ in range(n)]
    edge = band(noise, 900, 4200)
    air = band(noise, 180, 2400)
    grit = band(noise, 90, 950)
    plate = band(rec, 120, 4800)
    if name in ("shep_grab_tell","shep_grip","shep_motion","shep_seal"):
        plate=mechanism_source("cyl_close_rec")
    elif name in ("shep_sweep_tell","shep_sweep","shep_lift"):
        plate=mechanism_source("pump_rec")
    elif name in ("shep_counter","shep_break","shep_wound_heavy"):
        plate=mechanism_source("sword_steel_"+str(variant))
    phases = [rng.uniform(-.15, .15) for _ in range(5)]
    pitch = 1 + (variant-1)*.018

    def texture(t, speed=1.):
        pos = t * SR * speed * pitch
        j = int(pos)
        if j < 0 or j+1 >= len(plate): return 0.
        f = pos-j
        return plate[j]*(1-f)+plate[j+1]*f

    def strike(t, freq, decay, speed=.75):
        if t < 0: return 0.
        recorded=texture(t,speed)*math.exp(-t*(24 if name.startswith("shep_") else 7))
        if name.startswith("shep_"):recorded*=max(0.,min(1.,(.075-t)/.015))
        return (.65*recorded+.3*metal(t*pitch,freq,decay,phases))*min(1.,t/.0008)

    def crystal(t,freq,decay=15):
        if t<0:return 0.
        u=t*pitch
        return (math.sin(TAU*freq*u+.65*math.sin(TAU*freq*1.414*u))+.32*math.sin(TAU*freq*2.71*u))*math.exp(-t*decay)*min(1.,t/.002)

    def ping(t,freq,decay):
        # A bright inharmonic metal ping in the 2-5 kHz presence band: the
        # part of a warning that survives gunfire (tools/listen.py masking).
        if t<0:return 0.
        f=freq*pitch
        return (math.sin(TAU*f*t)+.6*math.sin(TAU*f*1.513*t+phases[0])+.35*math.sin(TAU*f*2.296*t+phases[1]))*math.exp(-t*decay)*min(1.,t/.0003)

    def bite(t,source,speed,decay):
        # Recorded high band, same onset as the strike it brightens.
        if t<0:return 0.
        hi=bright_source(source);pos=t*SR*speed*pitch;j=int(pos)
        if j+1>=len(hi):return 0.
        f=pos-j
        # Capped at 60 ms: the recording's later bounce would add a false pulse to the count
        return (hi[j]*(1-f)+hi[j+1]*f)*math.exp(-t*decay)*min(1.,t/.0003)*max(0.,min(1.,(.06-t)/.015))

    def chirp(t,f0,f1,length,decay):
        # A falling pitched pull, not noise: the phase of a glide f0 -> f1.
        if t<0:return 0.
        u=min(t,length);k=math.log(f1/f0)/length
        ph=TAU*f0*pitch*((math.exp(k*u)-1)/k+(t-u)*math.exp(k*length))
        return math.sin(ph)*math.exp(-t*decay)*min(1.,t/.0005)

    out = []
    for i in range(n):
        t = i / SR
        tt = t * pitch
        snap = edge[i]*math.exp(-t*110)
        v = 0.
        if name == "shep_fist_tell":
            # Two hydraulic catches, then a short chest resonance. No air bed.
            for start, freq in ((0, 165), (.21, 112)):
                u = t-start
                if u >= 0:
                    v += strike(u,freq,22,.55)*.9 + body(u,210,80,30)*.35
                    v += bite(u,"clank_rec",.9,FIST_BITE[1])*FIST_BITE[0]   # the catch's own steel edge; same pitch both times
            v+=metal(tt,320,16,phases)*.12
        elif name == "shep_sweep_tell":
            # An accelerating chain winch: four teeth, climbing in register.
            for k,start in enumerate((0,.17,.285,.365)):
                if t>=start:v+=strike(t-start,310+k*95,46,1.1)*(.8-k*.06)   # (user audition: the original winch beat any added layer)
        elif name == "shep_grab_tell":
            # Three finger latches, deliberately separated from the winch.
            for k, start in enumerate((0, .09, .30)):
                u = t-start
                if u >= 0: v += strike(u,790-k*220,65,1.3)*(.8-k*.1)+body(u,320,150,55)*.12+ping(u,(3900,3250,2600)[k],62)*.3   # closing
        elif name in ("shep_gaze_tell", "shep_beam_tell"):
            beam = name == "shep_beam_tell"
            # Gaze is a glassy high dyad; the travelling beam climbs three low rungs.
            for k,start in enumerate((0,.19,.38) if beam else (0,.16)):
                u = t-start
                if u >= 0:
                    v+=crystal(u,(180+k*90) if beam else (1040 if k==0 else 1470),18 if beam else 12)*.5
                    if beam:v+=ping(u,1800+k*450,30)*.26   # rungs climb
        elif name in ("shep_lock", "shep_parry", "shep_memory", "shep_moon"):
            freq = {"shep_lock":940,"shep_parry":1480,"shep_memory":410,"shep_moon":790}[name]
            if name=="shep_lock":v=crystal(t,1500,65)*.5+strike(t,940,70,1.3)*.4
            if name=="shep_parry":v=crystal(t,1810,36)*.6+math.sin(TAU*2715*tt)*math.exp(-t*45)*.15
            if name=="shep_memory":v=body(tt,420,260,35)*.5+metal(tt,570,45,phases)*.25+ping(t,2150,75)*.4   # a bright registration tick
            if name=="shep_moon":v=crystal(t,760,15)*.55+body(tt,300,120,30)*.2
        elif name == "shep_gather":
            # The keeper counts inward: three descending pressure knocks.
            for k,start in enumerate((0,.25,.50)):
                u=t-start
                if u>=0:v+=body(u,(150,120,95)[k],(85,65,46)[k],16)*.55+metal(u*pitch,235-k*40,20,phases)*.22+bite(u,"clank_rec",.62,GATHER_BITE[1])*GATHER_BITE[0]   # slowed clank: heavier, darker metal
        elif name == "shep_strain":
            env=(.2+.8*t/duration)*(.8+.2*math.sin(TAU*9*tt))
            v=(math.sin(TAU*146*tt)+.55*math.sin(TAU*153*tt)+.2*math.sin(TAU*220*tt))*env*.25
        elif name in ("shep_gaze_fire", "shep_beam"):
            beam = name == "shep_beam"
            env = math.exp(-t*(7 if beam else 23))
            v=crystal(t,430 if beam else 1240,7 if beam else 26)*.6+body(tt,310,92,24)*.45
            v+=snap*.08
        elif name == "ring_knock":
            for start, gain in ((0,1.),(.21,.74)):
                u = t-start
                if u >= 0: v += (strike(u,170,32,.42)+body(u,190,105,45)*.14)*gain
        elif name == "ring_creak":
            env = (.2+math.sin(math.pi*t/duration)**2)*(.6+.4*math.sin(TAU*11*tt)**2)
            v = (grit[i]*.6+air[i]*.2)*env
            v += math.sin(TAU*tt*116)*env*.055 + strike(t,180,14,.35)*.18
        elif name == "ring_sweep_warn":
            for start in (0,.24):
                u = t-start
                if u >= 0: v += strike(u,240,15,.5)*.8+body(u,150,72,18)*.18
            v += grit[i]*math.exp(-t*6)*.3
        elif name == "ring_sweep_pass":
            env = (.2+5*t)*math.exp(-t*6)
            v = (grit[i]*.8+air[i]*.7)*env + body(tt,130,42,9)*.2
        elif name == "ring_debris":
            for k, start in enumerate((0,.06,.13,.24,.39,.51)):
                u = t-start
                if u >= 0: v += strike(u,720+k*113,65,1.3)*(.5-k*.065)
        elif name == "shep_contact":
            v = strike(t,145,5,.48)*.9+body(tt,210,52,9)*.7+snap*.38
            v += metal(tt,285,3.8,phases)*.28
            v += math.sin(TAU*98*tt)*math.exp(-t*5)*.16
            for start, gain in ((.075,.22),(.19,.12)):
                if t>=start:v+=strike(t-start,390,6,.6)*gain
        elif name == "shep_sweep":
            # The pass is a torn chain and carriage thud, never a wind rush.
            for k,start in enumerate((0,.045,.11,.20,.32)):
                if t>=start:v+=strike(t-start,620-k*85,38,.8)*(.75-k*.1)
            v+=body(tt,195,80,11)*.4+metal(tt,210,4,phases)*.18
        elif name == "shep_grip":
            v=strike(t,325,35,.9)*.9+body(tt,250,85,22)*.5+metal(tt,230,6,phases)*.25
            if t>.075:v+=strike(t-.075,180,45,.65)*.45
        elif name == "shep_pin":
            v = strike(t,690,32,1.15)*.55+body(tt,330,135,36)*.5+snap*.12
            v += metal(tt,480,9,phases)*.18
        elif name == "shep_counter":
            v = strike(t,176,4,.55)*.75+body(tt,235,57,8)*.6
            v += metal(tt,370,3.2,phases)*.25+crystal(t,880,4)*.14
            if t >= .065: v += strike(t-.065,880,24,1.3)*.4
        elif name == "shep_break":
            for k, start in enumerate((0,.11,.29,.48)):
                u = t-start
                if u >= 0: v += strike(u,310-k*44,4,.7-k*.1)*(.8-k*.14)
            v += body(tt,155,43,5)*.3
            v += metal(tt,435,2.8,phases)*.15
        elif name == "shep_seal":
            v = strike(t,410,35,.8)+body(tt,220,95,30)*.3
            if t >= .065: v += strike(t-.065,190,32,.5)*.4
        elif name == "shep_implode":
            v = strike(t,95,4,.3)*.7+body(tt,140,39,8)*.65
            for k, start in enumerate((.07,.14,.26)):
                if t >= start: v += strike(t-start,340-k*60,14,.6)*(.35-k*.06)
            u = t-.22
            if u > 0: v += math.sin(TAU*49*u)*(1-math.exp(-u*30))*math.exp(-u*4)*.25
            v += grit[i]*math.exp(-t*2.5)*.28+metal(tt,210,2.3,phases)*.18
        elif name in ("shep_wound","shep_wound_heavy"):
            heavy=name=="shep_wound_heavy"
            v=strike(t,235 if heavy else 480,6 if heavy else 15,.65 if heavy else 1.1)*.8
            v+=body(tt,310 if heavy else 390,82 if heavy else 150,12 if heavy else 25)*.55
            # The reward: a bright armour crack over a shorter ring, so a hit
            # on the giant never sounds like the giant hitting the floor.
            v+=snap*.5+metal(tt,610,6 if heavy else 16,phases)*.2+ping(t,3400 if heavy else 4100,45 if heavy else 60)*.36
            if heavy:v+=metal(tt,350,3.3,phases)*.3+ping(t-.035,2700,40)*.22   # keeps its structural ring
        elif name in ("shep_motion","shep_lift"):
            lift=name=="shep_lift"
            for k,start in enumerate((0,.065,.145) if lift else (0,.09)):
                u=t-start
                if u>=0:v+=strike(u,(230+k*55) if lift else (115+k*65),38,.75)*(.6-k*.12)+body(u,190,100,40)*.18
        else:
            raise ValueError(name)
        v *= min(1., t/.0005)*min(1., (duration-t)/.035)
        out.append(math.tanh(v*(2.2 if name in ("shep_grip","shep_pin","shep_gaze_fire","shep_grab_tell","shep_seal") else 1.25)))
    bright = (name.endswith("_tell") and name != "shep_sweep_tell") or name in ("shep_gather","shep_memory","shep_wound","shep_wound_heavy")
    out = lowpass(out, 11000 if bright else 6200)
    dc = lowpass(out, 25)
    return [(v-d)*min(1., (n-1-i)/(SR*.015)) for i,(v,d) in enumerate(zip(out,dc))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compare", type=Path, help="directory of older shep_ WAVs for a before/after reel at authored file levels")
    args = parser.parse_args()
    rec = recording()
    for name, (_, target) in CUES.items():
        for variant in range(3):
            save(name+(f"_{variant}" if variant else ""), make(name,variant,rec), target)
    root = Path(__file__).resolve().parents[1]
    dest = root/"build"/"shepherd-audio"
    dest.mkdir(parents=True,exist_ok=True)
    pcm = bytearray()
    lines = ["# Chapter 6 audio audition", "",
             "- [Before / after](shepherd-comparison.wav), when generated with `--compare`; [timecodes](comparison.md).",
             "- [Gameplay capture](live-playthrough.wav), when recorded with `OVERDRIVE_AUDIO_AUDIT`: actual walking, firing, contacts, giant animation and phase changes.",
             "- [Attack language](attack-language.wav): isolated idle, fist, sweep, grab, gaze, gather and counter. [Timecodes](attack-language.md).",
             "- [Boss mix](shepherd-mix.wav): footsteps, giant movement, hits, warnings and counters; ending release at 31.5 s.",
             "- [Stress mix](shepherd-stress.wav): revolver, shotgun, rifle, and Overdrive from 18 to 25 s.",
             "- [Ring crossing](ring-crossing.wav): capsule/reef contacts, warning, countdown and debris.",
             "- Native callback captures: [court](native-court.wav), [crossing](native-crossing.wav), when recorded separately.",
             "", "The cue reel below uses each base variant, separated by 0.6 seconds. Original asset levels; no music.", ""]
    for name, (duration, _) in CUES.items():
        lines.append(f"- {len(pcm)/(SR*2):05.2f}s: {name} ({duration:.2f}s)")
        with wave.open(str(root/"assets"/"sfx"/(name+".wav"))) as w: pcm.extend(w.readframes(w.getnframes()))
        pcm.extend(bytes(int(.6*SR)*2))
    with wave.open(str(dest/"shepherd-cues.wav"),"wb") as w:
        w.setparams((1,2,SR,0,"NONE","not compressed"));w.writeframes(pcm)
    (dest/"README.md").write_text("\n".join(lines)+"\n")
    if args.compare:
        pcm = bytearray()
        lines = ["# Chapter 6 before / after", "", "Each pair plays BEFORE, then AFTER at its authored file level. Idle mechanisms now target -18 dBFS rather than -13; attack targets are unchanged. No music or extra normalization.", ""]
        for name in CUES:
            old = args.compare/(name+".wav")
            if not old.exists(): continue
            for label, path in (("BEFORE",old),("AFTER",root/"assets"/"sfx"/(name+".wav"))):
                lines.append(f"- {len(pcm)/(SR*2):05.2f}s: {name} {label}")
                with wave.open(str(path)) as w:
                    assert (w.getnchannels(),w.getsampwidth(),w.getframerate()) == (1,2,SR)
                    pcm.extend(w.readframes(w.getnframes()))
                pcm.extend(bytes(int(.5*SR)*2))
        with wave.open(str(dest/"shepherd-comparison.wav"),"wb") as w:
            w.setparams((1,2,SR,0,"NONE","not compressed"));w.writeframes(pcm)
        (dest/"comparison.md").write_text("\n".join(lines)+"\n")


if __name__ == "__main__": main()
