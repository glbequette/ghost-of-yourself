import pathlib, json, sys, wave
import numpy as np

raw=pathlib.Path(sys.argv[1])
root=pathlib.Path(__file__).resolve().parents[1]
lines=json.loads((raw/'Lines.json').read_text(encoding='utf-8-sig'))
rate=48000
parts=[np.zeros(int(.4*rate))];time=.4;cues=[]
for index,text in enumerate(lines):
    with wave.open(str(raw/f'Line{index}.wav'),'rb') as f:
        assert f.getsampwidth()==2
        channels=f.getnchannels();source_rate=f.getframerate()
        data=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').astype(np.float64)/32768
        data=data.reshape(-1,channels).mean(axis=1)
    # Trim synthesis tail silence, retaining breathing room between sentences.
    audible=np.where(np.abs(data)>.002)[0]
    if len(audible):data=data[max(0,audible[0]-int(.06*source_rate)):min(len(data),audible[-1]+int(.12*source_rate))]
    count=int(len(data)*rate/source_rate)
    data=np.interp(np.arange(count)*source_rate/rate,np.arange(len(data)),data)
    t=np.arange(len(data))/rate
    # Subtle robotic texture: a quiet short-delay copy and gentle ring modulation.
    delay=int(.012*rate)
    delayed=np.concatenate([np.zeros(delay),data[:-delay]]) if len(data)>delay else np.zeros_like(data)
    data=(data+.13*delayed)*(.94+.06*np.sin(2*np.pi*38*t))
    cues.append({'time':round(time,4),'text':text})
    parts.extend([data,np.zeros(int(.55*rate))]);time+=(len(data)/rate+.55)
combined=np.concatenate(parts)
combined*=.87/max(.01,float(np.abs(combined).max()))
audio=root/'Audio'/'Intro_RobotWelcome.wav';audio.parent.mkdir(exist_ok=True)
with wave.open(str(audio),'wb') as f:
    f.setnchannels(1);f.setsampwidth(2);f.setframerate(rate)
    f.writeframes(np.round(combined*32767).astype('<i2').tobytes())
(root/'Narration.json').write_text(json.dumps({'durationSeconds':len(combined)/rate,
    'voice':'Microsoft Zira Desktop with subtle original robot processing',
    'sampleRate':rate,'channels':1,'bits':16,'cues':cues},indent=2))
(root/'Story_Script.txt').write_text('\n\n'.join(lines))
assert np.isfinite(combined).all() and np.abs(combined).max()<1
print(f'Voice generated: {len(combined)/rate:.1f} seconds, mono 48 kHz 16-bit PCM, no clipping')
