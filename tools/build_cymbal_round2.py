#!/usr/bin/env python3
"""Round 2: compare independent real strikes, retaining the initial transient."""
import concurrent.futures, hashlib, json, math, struct, urllib.request
from pathlib import Path
from build_cymbal_auditions import read_pcm,resample,fade,wav,ima_encode,ima_decode,clamp

V="https://raw.githubusercontent.com/sfzinstruments/virtuosity_drums/9f04cf9a734527edfbb0a4eee1f674e45bbf71bc/"
U="https://raw.githubusercontent.com/tidalcycles/uzu-drumkit/2f3e05c70ab4d73ad053a1467adec89bd27377a0/"
# Independent microphone channels and round robins, not alternate codecs of one hit.
candidates={
 "crash":[
 ("oh1",V+"Samples/oh/crash/oh_crash_crash_vl3_rr1.flac","464020e19ae66f3c6b1bedcb87877590f705cdff"),
 ("oh2",V+"Samples/oh/crash/oh_crash_crash_vl3_rr2.flac","f80c919d56e5a7229b6eec4a06a8a5b5b4739ba7"),
 ("oh3",V+"Samples/oh/crash/oh_crash_crash_vl3_rr3.flac","28eac522cce5b49d12e6940ce25af633c266df04"),
 ("oh4",V+"Samples/oh/crash/oh_crash_crash_vl3_rr4.flac","b50d5541a7af1b0be0a7ba8657d8562977cd161b"),
 ("close2",V+"Samples/snaremic/crash/snaremic_crash_crash_vl3_rr2.flac","07e929c4eb63fd1bf1b80c42e0972f44e35730ce"),
 ("close3",V+"Samples/snaremic/crash/snaremic_crash_crash_vl3_rr3.flac","e143302cf5653304275921dc7133c40705d08906"),
 ("uzu",U+"cr/11_cr_mot4i.wav","b22925e500f6f2366251591b4eadd18712583e3b"),
 ],
 "ride":[
 ("oh1",V+"Samples/oh/ride/oh_ride_ride_vl3_rr1.flac","11b81972377915468bcf033785d313d2a7665b31"),
 ("oh2",V+"Samples/oh/ride/oh_ride_ride_vl3_rr2.flac","7eb3b0ef766a8fe81cb6e7c9517a295731982a04"),
 ("oh3",V+"Samples/oh/ride/oh_ride_ride_vl3_rr3.flac","c44542f02348ac68fc285c31c7360b1de15232f6"),
 ("oh4",V+"Samples/oh/ride/oh_ride_ride_vl3_rr4.flac","bfa713d5679989101fba959bf591a133e8e0b992"),
 ("close2",V+"Samples/snaremic/ride/snaremic_ride_ride_vl3_rr2.flac","91944475244b01a16aa082461e11e7f16e139384"),
 ("bell2",V+"Samples/oh/ride/oh_ride_bell_vl3_rr2.flac","e717207aebfc47fbd298daab5a5256013e34b9f8"),
 ("uzu",U+"rd/10_rd_switchangel.wav","31b97963d0e9fccd96e7521fd13e09f05facc456"),
 ]}
OUT=Path("build/cymbal-round2"); CACHE=Path("build/cymbal-sources-round2")
def rms(p):return math.sqrt(sum(x*x for x in p)/max(1,len(p)))
def analyze(job):
    kind,(label,url,expected)=job
    f=CACHE/(kind+"_"+label+Path(url).suffix)
    if not f.exists():
        with urllib.request.urlopen(url,timeout=90) as r:f.write_bytes(r.read())
    raw=f.read_bytes()
    assert hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()==expected
    x=read_pcm(f)
    peak=max(abs(y) for y in x)
    # Use sustained 1 ms energy rather than a single noisy sample to detect the strike.
    n=44
    energies=[rms(x[i:i+n]) for i in range(0,len(x),n)]
    threshold=max(energies)*0.025
    first=next(i for i,y in enumerate(energies) if y>=threshold)
    start=max(0,first*n-44)
    x=x[start:];g=28000/max(abs(y) for y in x)
    x=[clamp(round(y*g),-32768,32767) for y in x]
    w=220 # ~5 ms
    e=[rms(x[i:i+w]) for i in range(0,min(len(x),8820),w)]
    strongest=max(e)
    early=max(e[:5])
    peak_ms=e.index(strongest)*w*1000/44100
    score=early/max(1,strongest)
    meta=dict(instrument=kind,label=label,source=url,git_blob=expected,license="Unlicense" if url.startswith(U) else "CC0-1.0",trim_ms=round(start*1000/44100,2),early_to_peak=round(score,3),peak_ms=round(peak_ms,2))
    return meta,x
def main():
    OUT.mkdir(parents=True,exist_ok=True);CACHE.mkdir(parents=True,exist_ok=True)
    jobs=[(kind,item) for kind,items in candidates.items() for item in items]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool: results=list(pool.map(analyze,jobs))
    manifest={"all_candidates":[m for m,x in results],"selected":[]}
    # Always include contrasting independent Uzu source plus strongest overhead and close/bell.
    for kind in candidates:
        group=[(m,x) for m,x in results if m["instrument"]==kind]
        pick=lambda prefix:max((p for p in group if p[0]["label"].startswith(prefix)),key=lambda p:(p[0]["early_to_peak"],-p[0]["peak_ms"]))
        selections=[pick("oh"),pick("close"),pick("uzu")]
        if kind=="ride":selections.append(pick("bell"))
        for m,x in selections:
            stem=kind+"_"+m["label"]
            ref=fade(x[:44100*3],44100,.15)
            wav(OUT/(stem+"_REF.wav"),ref,44100)
            duration=1.15 if kind=="crash" else .95
            # Standard compressed candidate: same source and no fade-in.
            pcm=fade(resample(x[:round(duration*44100)],22050),22050,.14)
            encoded=ima_encode(pcm);dec=ima_decode(encoded,len(pcm))
            wav(OUT/(stem+"_A_22k_ADPCM.wav"),dec,22050)
            (OUT/(stem+"_A.bin")).write_bytes(encoded)
            # Attack-preserving mixed-rate payload: first 30ms full-rate PCM16,
            # remaining tail ADPCM half-rate. Split on an even sample.
            n=1324
            full=fade(x[:round(duration*44100)],44100,.14)
            attack=full[:n];tail=resample(full[n:],22050)
            assert tail
            pack=ima_encode(tail);decoded=ima_decode(pack,len(tail))
            high=[]
            for i,y in enumerate(decoded):
                nxt=decoded[min(i+1,len(decoded)-1)]
                high.extend((y,(y+nxt)//2))
            payload=struct.pack("<"+"h"*len(attack),*attack)+pack
            (OUT/(stem+"_H.bin")).write_bytes(payload)
            wav(OUT/(stem+"_H_full_attack.wav"),attack+high[:len(full)-n],44100)
            manifest["selected"].append({**m,"A_bytes":len(encoded),"A_frames":len(pcm),"H_bytes":len(payload),"H_attack_frames":n,"H_tail_frames":len(tail)})
    (OUT/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    lines=["MASHINA cymbal audition round 2","Different source strikes and microphones, no fade-in.","REF: source with end fade only; A: 22.05 kHz ADPCM; H: first 30 ms original 44.1 kHz PCM16, then 22.05 kHz ADPCM with linear interpolation.","Compare REF files first. Pick the most convincing crash and ride, then choose A or H. BINs are sample payloads, not firmware.","","Peak timing is a 5 ms RMS measurement; early/peak ratio uses strongest first 25ms vs first 200ms. It is a screening metric, not a subjective quality judgment.",""]
    for m in manifest["selected"]:
        lines.append(f'{m["instrument"]}_{m["label"]}: peak {m["peak_ms"]:.1f} ms; early/peak {m["early_to_peak"]:.3f}; A {m["A_bytes"]} bytes; H {m["H_bytes"]} bytes')
    lines+=["","Virtuosity Drums: Versilian Studios / Karoryfer Samples, CC0-1.0; Uzu drumkit: The Unlicense. Exact pinned source and blob identifiers are in manifest.json.","The ride bell is an optional contrasting articulation, not necessarily the intended bow sound."]
    (OUT/"READ-ME.txt").write_text("\n".join(lines)+"\n")
    print("\n".join(lines))
if __name__=="__main__":main()
