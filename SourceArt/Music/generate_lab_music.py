"""Original 90 BPM ambient pulse, synthesized as a seamless stereo loop."""
from pathlib import Path
import json
import wave
import numpy as np

RATE = 48000
BEAT = 32000  # 90 BPM, exactly integer samples
FRAMES = BEAT * 64
audio = np.zeros((FRAMES, 2), dtype=np.float64)
rng = np.random.default_rng(3106)

def freq(note):
    return 440.0 * 2 ** ((note - 69) / 12)

def mix(signal, start, gain, pan=0.0):
    positions = (np.arange(len(signal)) + int(start)) % FRAMES
    left = np.sqrt((1 - pan) / 2)
    right = np.sqrt((1 + pan) / 2)
    np.add.at(audio[:, 0], positions, signal * gain * left)
    np.add.at(audio[:, 1], positions, signal * gain * right)

chords = [(38, [62, 65, 69, 76]), (34, [58, 62, 65, 69]),
          (41, [57, 60, 64, 67]), (36, [60, 64, 67, 74])]
for section, (root, notes) in enumerate(chords):
    start = section * BEAT * 16
    t = np.arange(BEAT * 16 + RATE * 2) / RATE
    sustain = BEAT * 16 / RATE
    envelope = (1 - np.exp(-t / 1.1)) * np.clip((sustain + 2 - t) / 2, 0, 1)
    for voice, note in enumerate(notes):
        f = freq(note)
        tone = (np.sin(2*np.pi*f*t) + .32*np.sin(2*np.pi*f*1.0012*t + .6)
                + .09*np.sin(2*np.pi*f*2*t))
        mix(tone * envelope, start, .065, (voice - 1.5) * .18)

pattern = [0, 2, 1, 3, 2, 1, 0, 2]
for step in range(128):
    root, notes = chords[step // 32]
    t = np.arange(int(RATE * 2.2)) / RATE
    f = freq(notes[pattern[step % 8]])
    envelope = (1 - np.exp(-t * 38)) * np.exp(-t * 3.4)
    envelope *= np.clip((2.2-t)/.15, 0, 1)
    chime = np.sin(2*np.pi*f*t + .22*np.sin(2*np.pi*f*2*t))
    chime += .12*np.sin(2*np.pi*f*3*t) * np.exp(-t*9)
    mix(chime * envelope, step * BEAT // 2, .085, .32*np.sin(step*1.1))

for beat in range(64):
    root = chords[beat // 16][0]
    t = np.arange(int(RATE * .6)) / RATE
    envelope = (1-np.exp(-t*75))*np.exp(-t*7)*np.clip((.6-t)/.07, 0, 1)
    bass = np.sin(2*np.pi*freq(root)*t) + .08*np.sin(4*np.pi*freq(root)*t)
    mix(bass * envelope, beat*BEAT, .12, 0)
    if beat % 2 == 0:
        kt = np.arange(int(RATE*.2))/RATE
        kick = np.sin(2*np.pi*(42*kt + 16*.045*(1-np.exp(-kt/.045))))
        mix(kick * (1-np.exp(-kt*600))*np.exp(-kt*32)*np.clip((.2-kt)/.03,0,1), beat*BEAT, .045)
    nt = np.arange(int(RATE*.07))/RATE
    noise = rng.standard_normal(len(nt))
    high = np.concatenate(([0], np.diff(noise)))
    mix(high * np.sin(np.pi*nt/.07)**2 * np.exp(-nt*60), beat*BEAT+BEAT//2, .003, .35)

audio -= audio.mean(axis=0)
audio *= .48 / np.max(np.abs(audio))
pcm = np.rint(audio * 32767).astype('<i2')
out = Path(__file__).with_name('GOY_LabMusic.wav')
with wave.open(str(out), 'wb') as wav:
    wav.setnchannels(2)
    wav.setsampwidth(2)
    wav.setframerate(RATE)
    wav.writeframes(pcm.tobytes())
metrics = dict(duration_seconds=FRAMES/RATE, bpm=90, sample_rate=RATE,
               channels=2, peak=float(np.max(np.abs(audio))),
               rms=float(np.sqrt(np.mean(audio**2))),
               boundary_step=float(np.max(np.abs(audio[0]-audio[-1]))),
               normal_max_step=float(np.max(np.abs(np.diff(audio, axis=0)))))
out.with_suffix('.json').write_text(json.dumps(metrics, indent=2))
print(json.dumps(metrics))
