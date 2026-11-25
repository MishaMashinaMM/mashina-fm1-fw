#!/usr/bin/env python3
"""Build a reproducible, level-matched cymbal listening pack, not firmware."""
import argparse, hashlib, json, math, struct, subprocess, urllib.request, wave
from pathlib import Path

SOURCE = "9f04cf9a734527edfbb0a4eee1f674e45bbf71bc"
SOURCES = {
    "crash": ("Samples/mid/crash/mid_crash_crash_vl3_rr1.flac", "da7899acee1a1d9516a5732a820db08084e83501"),
    "ride": ("Samples/mid/ride/mid_ride_ride_vl3_rr1.flac", "69c44df7ded88b4c82620527cf7a5c117bcac301"),
}
BASE = "https://raw.githubusercontent.com/sfzinstruments/virtuosity_drums/" + SOURCE + "/"
STEPS = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
INDEX = [-1,-1,-1,-1,2,4,6,8]
def clamp(x, lo, hi): return max(lo,min(hi,x))
def step_decode(pred, idx, code):
    s=STEPS[idx]
    d=(s>>3)+((s>>2) if code&1 else 0)+((s>>1) if code&2 else 0)+(s if code&4 else 0)
    return clamp(pred-d if code&8 else pred+d,-32768,32767), clamp(idx+INDEX[code&7],0,88)
def ima_encode(pcm):
    # Explicit raw format: first predictor, index, reserved; low nibble first.
    pred=int(pcm[0]); idx=0
    out=bytearray(struct.pack("<hBB",pred,idx,0))
    for i,target in enumerate(pcm[1:]):
        d=int(target)-pred; code=8 if d<0 else 0; d=abs(d); s=STEPS[idx]
        for flag,threshold in ((4,s),(2,s>>1),(1,s>>2)):
            if d>=threshold: code|=flag; d-=threshold
        pred,idx=step_decode(pred,idx,code)
        if i%2==0: out.append(code)
        else: out[-1]|=code<<4
    return bytes(out)
def ima_decode(data,count):
    pred,idx,_=struct.unpack("<hBB",data[:4]); out=[pred]
    for byte in data[4:]:
        for c in (byte&15,byte>>4):
            if len(out)>=count: return out
            pred,idx=step_decode(pred,idx,c);out.append(pred)
    assert len(out)==count
    return out
def mulaw_encode(pcm):
    # 8-bit logarithmic companding, scale fixed to 16-bit full range.
    return bytes(clamp(round(127.5*(1+math.copysign(math.log1p(255*abs(x)/32768)/math.log(256),x))),0,255) for x in pcm)
def mulaw_decode(data):
    return [clamp(round(math.copysign(32768*math.expm1(abs(2*x/255-1)*math.log(256))/255,2*x/255-1)),-32768,32767) for x in data]
def read_pcm(path):
    raw=subprocess.check_output(["ffmpeg","-v","error","-i",str(path),"-ac","1","-ar","44100","-f","s16le","-"])
    return list(struct.unpack("<"+"h"*(len(raw)//2),raw))
def resample(pcm,rate):
    if rate==44100:return list(pcm)
    raw=subprocess.run(["ffmpeg","-v","error","-f","s16le","-ac","1","-ar","44100","-i","-","-ar",str(rate),"-f","s16le","-"],input=struct.pack("<"+"h"*len(pcm),*pcm),capture_output=True,check=True).stdout
    return list(struct.unpack("<"+"h"*(len(raw)//2),raw))
def fade(pcm,rate,duration):
    n=min(len(pcm),round(duration*rate))
    if n>1:
        for i in range(n):pcm[len(pcm)-n+i]=round(pcm[len(pcm)-n+i]*(0.5+0.5*math.cos(math.pi*i/(n-1))))
    return pcm
def wav(path,pcm,rate):
    with wave.open(str(path),"wb") as f:
        f.setparams((1,2,rate,len(pcm),"NONE","not compressed"))
        f.writeframes(struct.pack("<"+"h"*len(pcm),*pcm))
def get_source(cache,name):
    path,sha=SOURCES[name]; file=cache/(name+".flac")
    if not file.exists():
        with urllib.request.urlopen(BASE+path,timeout=60) as r:file.write_bytes(r.read())
    raw=file.read_bytes()
    actual=hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()
    if actual!=sha:raise ValueError(f"Source hash mismatch for {name}: {actual}")
    return file
def main():
    p=argparse.ArgumentParser();p.add_argument("--out",type=Path,default=Path("build/cymbal-auditions"));p.add_argument("--cache",type=Path,default=Path("build/cymbal-sources"));a=p.parse_args()
    a.out.mkdir(parents=True,exist_ok=True);a.cache.mkdir(parents=True,exist_ok=True)
    # Test nibble ordering, odd/even lengths, clamping, and determinism.
    assert ima_decode(bytes([0,0,0,0,0x71]),3)==[0,1,12]
    for pcm in ([0],[1,2],[0,32767,-32768,3000],list(range(-100,100))):
        data=ima_encode(pcm);assert len(data)==4+len(pcm)//2
        assert len(ima_decode(data,len(pcm)))==len(pcm)
    rows=[]
    variants=[
        ("A_22k_ADPCM",22050,1.2,1.0,"ima"),
        ("B_32k_ADPCM",32000,.9,.8,"ima"),
        ("C_44k_ADPCM",44100,.65,.6,"ima"),
        ("D_22k_companded8",22050,.65,.55,"mulaw"),
        ("E_16k_ADPCM",16000,1.6,1.4,"ima"),
    ]
    for name in SOURCES:
        original=read_pcm(get_source(a.cache,name))
        peak=max(abs(x) for x in original)
        # Find first meaningful transient, preserve 3 ms of pre-roll.
        first=next(i for i,x in enumerate(original) if abs(x)>peak*.015)
        original=original[max(0,first-132):]
        gain=28000/max(abs(x) for x in original)
        original=[clamp(round(x*gain),-32768,32767) for x in original]
        reference=fade(original[:4*44100],44100,.15)
        wav(a.out/(name+"_REFERENCE.wav"),reference,44100)
        for label,rate,crash,ride,codec in variants:
            seconds=crash if name=="crash" else ride
            pcm=fade(resample(original[:round(seconds*44100)],rate),rate,min(.22,seconds*.22))
            payload=ima_encode(pcm) if codec=="ima" else mulaw_encode(pcm)
            decoded=ima_decode(payload,len(pcm)) if codec=="ima" else mulaw_decode(payload)
            prefix=name+"_"+label
            wav(a.out/(prefix+".wav"),decoded,rate)
            (a.out/(prefix+".bin")).write_bytes(payload)
            rows.append(dict(instrument=name,variant=label,rate=rate,frames=len(pcm),bytes=len(payload),codec=codec,sha256=hashlib.sha256(payload).hexdigest()))
    (a.out/"manifest.json").write_text(json.dumps({"source_commit":SOURCE,"source_license":"CC0-1.0","sources":SOURCES,"variants":rows},indent=2)+"\n")
    lines=["MASHINA DRUM-AC cymbal audition, round 1","Source: Virtuosity Drums, Versilian Studios / Karoryfer Samples (CC0-1.0).","https://github.com/sfzinstruments/virtuosity_drums/tree/"+SOURCE,"","Compare each instrument's REFERENCE first, then A–E. WAVs are decoded from the exact raw payloads, except REFERENCE. All variants of each instrument share one source recording and gain. End fades prevent abrupt cuts. Playback on a PC is not yet an FM1 resampler or a CPU benchmark.","","Combined stored payload sizes (excludes decoder and descriptors):"]
    for label,*_ in variants:
        size=sum(r["bytes"] for r in rows if r["variant"]==label)
        lines.append(f"{label}: {size:,} bytes ({size/1024:.2f} KiB)")
    lines+=["","The BINs are experimental payloads, not firmware. Do not flash them.","Please report the smallest convincing variant for crash and ride separately. Mix-and-match is possible."]
    (a.out/"READ-ME.txt").write_text("\n".join(lines)+"\n")
    print("\n".join(lines))
if __name__=="__main__":main()
