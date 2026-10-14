"""Generate original, royalty-free procedural art and ambient soundtrack assets."""
from pathlib import Path
import math, subprocess, wave
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent / "assets"
(ROOT / "textures").mkdir(parents=True, exist_ok=True)
(ROOT / "audio").mkdir(parents=True, exist_ok=True)
rng = np.random.default_rng(31855)

# High-detail repeatable floor material: dark alloy, etched panel grid and cyan seams.
size = 1024
noise = rng.normal(0, 4.4, (size, size, 1)).astype(np.float32)
y, x = np.mgrid[0:size, 0:size]
base = np.zeros((size,size,3), dtype=np.float32)
base[:,:,0] = 13 + noise[:,:,0]
base[:,:,1] = 20 + noise[:,:,0]*1.15
base[:,:,2] = 33 + noise[:,:,0]*1.7
# Broad panel shading and tiny random scratches.
panel_shade = ((x % 128) < 3) | ((y % 128) < 3)
base[panel_shade] += np.array([9, 20, 30],dtype=np.float32)
img = Image.fromarray(np.uint8(np.clip(base,0,255)), "RGB")
d = ImageDraw.Draw(img)
for xx in range(0,size,128):
    d.line((xx,0,xx,size), fill=(25,104,130), width=2)
    d.line((xx+5,0,xx+5,size), fill=(18,36,54), width=1)
for yy in range(0,size,128):
    d.line((0,yy,size,yy), fill=(23,86,112), width=2)
    d.line((0,yy+5,size,yy+5), fill=(18,35,53), width=1)
for _ in range(2600):
    xx=int(rng.integers(0,size)); yy=int(rng.integers(0,size)); length=int(rng.integers(2,28))
    col=(int(rng.integers(15,30)),int(rng.integers(25,45)),int(rng.integers(35,60)))
    d.line((xx,yy,min(size-1,xx+length),yy),fill=col,width=1)
for yy in range(0,size,128):
    for xx in range(0,size,128):
        d.rounded_rectangle((xx+8,yy+8,xx+14,yy+14), radius=2, fill=(68,212,238))
img.save(ROOT/"textures"/"floor.jpg", quality=92, optimize=True, progressive=True)

# Metallic panel texture for large drones/buildings.
size=768
noise=rng.normal(0,7,(size,size,1)).astype(np.float32)
y,x=np.mgrid[0:size,0:size]
base=np.zeros((size,size,3),dtype=np.float32)
base[:,:,0]=24+noise[:,:,0]
base[:,:,1]=34+noise[:,:,0]*1.05
base[:,:,2]=51+noise[:,:,0]*1.4
img=Image.fromarray(np.uint8(np.clip(base,0,255)),"RGB")
d=ImageDraw.Draw(img)
for yy in range(0,size,96):
    d.line((0,yy,size,yy), fill=(52,81,105), width=3)
for xx in range(0,size,128):
    d.line((xx,0,xx,size), fill=(41,67,90), width=2)
for yy in range(18,size,96):
    for xx in range(18,size,128):
        d.rectangle((xx,yy,xx+8,yy+8), fill=(88,178,201))
for _ in range(1800):
    xx=int(rng.integers(0,size)); yy=int(rng.integers(0,size)); length=int(rng.integers(2,22))
    d.line((xx,yy,xx+length,yy), fill=(32,47,66), width=1)
img.save(ROOT/"textures"/"metal.jpg", quality=91, optimize=True, progressive=True)


# Character/hostile/core UV textures: panelled armor, vents, bolts and energy seams.
def make_sci_texture(filename, base_color, accent_color, seed_offset, grid=128, scratches=1500):
    local_rng = np.random.default_rng(31855 + seed_offset)
    tex_size = 1024
    noise = local_rng.normal(0, 5.0, (tex_size, tex_size, 1)).astype(np.float32)
    rgb = np.empty((tex_size, tex_size, 3), dtype=np.float32)
    for channel in range(3):
        rgb[:,:,channel] = base_color[channel] + noise[:,:,0]*(0.8 + channel*0.18)
    img = Image.fromarray(np.uint8(np.clip(rgb, 0, 255)), "RGB")
    draw = ImageDraw.Draw(img)
    dark = tuple(max(0, int(c*0.48)) for c in base_color)
    mid = tuple(min(255, int(c*1.55)) for c in base_color)
    for yy in range(0, tex_size, grid):
        draw.line((0, yy, tex_size, yy), fill=dark, width=3)
        draw.line((0, yy+5, tex_size, yy+5), fill=mid, width=1)
    for xx in range(0, tex_size, grid):
        draw.line((xx, 0, xx, tex_size), fill=dark, width=3)
    for yy in range(0, tex_size, grid):
        for xx in range(0, tex_size, grid):
            # Corner bolts and a short circuit seam on every armor tile.
            for bx, by in ((xx+11, yy+11), (xx+grid-12, yy+11), (xx+11, yy+grid-12), (xx+grid-12, yy+grid-12)):
                draw.ellipse((bx-2, by-2, bx+2, by+2), fill=accent_color)
            if (xx//grid + yy//grid + seed_offset) % 2 == 0:
                draw.line((xx+grid*0.25, yy+grid*0.78, xx+grid*0.73, yy+grid*0.78), fill=accent_color, width=3)
                draw.line((xx+grid*0.73, yy+grid*0.78, xx+grid*0.73, yy+grid*0.60), fill=accent_color, width=2)
            else:
                draw.rectangle((xx+grid*0.28, yy+grid*0.27, xx+grid*0.72, yy+grid*0.31), fill=mid)
    for _ in range(scratches):
        xx=int(local_rng.integers(0,tex_size)); yy=int(local_rng.integers(0,tex_size))
        length=int(local_rng.integers(3,26))
        c=tuple(max(0,min(255,int(v*local_rng.uniform(0.55,1.2)))) for v in base_color)
        draw.line((xx,yy,min(tex_size-1,xx+length),yy),fill=c,width=1)
    img.save(ROOT/"textures"/filename, quality=91, optimize=True, progressive=True)

make_sci_texture("player_armor.jpg", (17, 36, 55), (67, 220, 255), 1, grid=128, scratches=2600)
make_sci_texture("drone_shell.jpg", (28, 22, 38), (255, 65, 161), 2, grid=96, scratches=2200)
make_sci_texture("boss_shell.jpg", (32, 15, 25), (255, 78, 83), 3, grid=160, scratches=3200)
make_sci_texture("core_circuit.jpg", (7, 28, 40), (75, 255, 184), 4, grid=128, scratches=1400)

# Cinematic city-at-night title artwork.
W,H=1920,1080
arr=np.zeros((H,W,3),dtype=np.uint8)
for yy in range(H):
    t=yy/H
    arr[yy,:,0]=int(4+8*t)
    arr[yy,:,1]=int(8+12*t)
    arr[yy,:,2]=int(23+28*t)
bg=Image.fromarray(arr,"RGB")
glow=Image.new("RGBA",(W,H),(0,0,0,0)); gd=ImageDraw.Draw(glow)
# huge eclipse/planet
cx,cy=1430,300
for rad in range(245,20,-8):
    alpha=int(max(1,22*(1-rad/260)))
    gd.ellipse((cx-rad,cy-rad,cx+rad,cy+rad),fill=(55,190,255,alpha))
gd.ellipse((cx-205,cy-205,cx+205,cy+205),fill=(8,12,32,235),outline=(74,210,245,220),width=5)
gd.arc((cx-225,cy-225,cx+225,cy+225),15,260,fill=(255,62,157,200),width=6)
# city skyline silhouettes and lit windows
for x0 in range(-20,W,36):
    h=int(rng.integers(150,430)); top=H-h
    width=int(rng.integers(20,45))
    gd.rectangle((x0,top,x0+width,H),fill=(4,8,19,230),outline=(20,46,73,150),width=2)
    if rng.random()<0.35: gd.rectangle((x0+width//3,top-20,x0+width*2//3,top),fill=(5,10,24,230))
    for wy in range(top+18,H-12,23):
        for wx in range(x0+5,x0+width-4,10):
            if rng.random()<0.20:
                gd.rectangle((wx,wy,wx+3,wy+6),fill=(int(rng.integers(35,100)),int(rng.integers(145,225)),int(rng.integers(190,255)),int(rng.integers(95,225))))
# vertical neon billboards and street streaks
for _ in range(60):
    xx=int(rng.integers(0,W)); yy=int(rng.integers(400,H-80)); ln=int(rng.integers(20,140))
    col=(int(rng.choice([50,80,230,255])),int(rng.choice([150,220,60,95])),int(rng.choice([255,250,180])),int(rng.integers(80,180)))
    gd.line((xx,yy,xx, min(H,yy+ln)), fill=col, width=int(rng.integers(1,4)))
for _ in range(180):
    xx=int(rng.integers(0,W)); yy=int(rng.integers(0,H)); rad=int(rng.integers(1,3))
    gd.ellipse((xx-rad,yy-rad,xx+rad,yy+rad),fill=(116,220,255,int(rng.integers(70,210))))
glow=glow.filter(ImageFilter.GaussianBlur(10))
bg=Image.alpha_composite(bg.convert("RGBA"),glow)
# crisp skyline on top
sharp=Image.new("RGBA",(W,H),(0,0,0,0)); sd=ImageDraw.Draw(sharp)
for x0 in range(-20,W,36):
    h=0 # skyline already in softened glow; add skyline outlines
# horizon energy line
sd.line((0,805,W,760),fill=(56,205,255,105),width=3)
sd.line((0,817,W,772),fill=(246,54,151,70),width=2)
bg=Image.alpha_composite(bg,sharp)
bg.convert("RGB").save(ROOT/"textures"/"title_bg.jpg",quality=91,optimize=True,progressive=True)

# Original ambient synthwave tracks, seamlessly faded at loop edges. Generated without samples.
sr=44100
duration=90.0
N=int(sr*duration)
t=np.arange(N,dtype=np.float32)/sr
notes_by_act=[55.0,58.27,65.41,49.0,41.20]
scales=[[0,3,7,10],[0,5,7,10],[0,3,5,8],[0,2,7,9],[0,3,6,10]]
for act in range(5):
    root=notes_by_act[act]
    scale=scales[act]
    # Slowly changing chord pad with detuned partials.
    chord_idx=np.floor(t/11.25).astype(np.int32)%4
    semitone_steps=[scale[0],scale[1],scale[2],scale[-1]]
    freqs=np.array([root*2**(st/12.0) for st in semitone_steps],dtype=np.float32)
    pad=np.zeros(N,dtype=np.float32)
    for voice,mul in enumerate([1.0,1.501,2.003]):
        f=freqs[chord_idx]*(mul if voice==0 else mul*0.5)
        pad += np.sin(2*np.pi*f*t + voice*0.7)/(voice+1.0)
    lfo=0.62+0.38*np.sin(2*np.pi*t/9.0+act)
    pad*=0.12*lfo
    # Arpeggiator, eight notes per 4.5-second phrase.
    arp=np.zeros(N,dtype=np.float32)
    step=0.28125
    phrase=np.floor(t/step).astype(np.int32)
    note_pattern=[0,2,1,3,2,1,3,2,0,1,2,3,1,2,0,3]
    note_num=np.array([note_pattern[int(i)%len(note_pattern)] for i in (phrase%len(note_pattern))],dtype=np.int32)
    arp_freq=np.array([root*4*2**(scale[n%len(scale)]/12.0) for n in range(len(scale))],dtype=np.float32)[note_num]
    local=t-phrase*step
    env=np.exp(-local*5.2)*(local<step)
    arp += 0.10*np.sin(2*np.pi*arp_freq*t)*env
    # Soft kick/snare pulses and filtered-ish hats made of enveloped noise.
    beat=0.72
    beat_idx=np.floor(t/beat).astype(np.int32)
    beat_local=t-beat_idx*beat
    kick_freq=78.0-32.0*np.minimum(beat_local/0.18,1.0)
    kick=np.sin(2*np.pi*kick_freq*t)*np.exp(-beat_local*18.0)*(beat_local<0.25)
    kick*=0.18
    sn_local=t-np.floor(t/(beat*2.0))*(beat*2.0)
    noise=rng.normal(0,1,N).astype(np.float32)
    snare=noise*np.exp(-sn_local*27.0)*(sn_local<0.12)*(beat_idx%4==2)*0.055
    hat_local=t-np.floor(t/(beat*0.5))*(beat*0.5)
    noise2=rng.normal(0,1,N).astype(np.float32)
    hats=noise2*np.exp(-hat_local*85.0)*(hat_local<0.035)*0.02
    # Gentle cinematic risers and one-note motif.
    riser=np.sin(2*np.pi*(330+110*np.sin(2*np.pi*t/22.5))*t)*0.025*(0.5+0.5*np.sin(2*np.pi*t/18.0))
    mono=pad+arp+kick+snare+hats+riser
    # fade endpoints so the soundtrack can loop cleanly
    fade_n=int(sr*1.8)
    fade=np.ones(N,dtype=np.float32)
    fade[:fade_n]=np.linspace(0,1,fade_n,dtype=np.float32)
    fade[-fade_n:]=np.linspace(1,0,fade_n,dtype=np.float32)
    mono=np.clip(mono*fade,-0.88,0.88)
    # Very subtle stereo movement.
    left=mono*(0.90+0.10*np.sin(2*np.pi*t/13.0+act))
    right=mono*(0.90+0.10*np.sin(2*np.pi*t/17.0+act+1.0))
    stereo=np.stack([left,right],axis=1)
    pcm=(stereo*32767).astype(np.int16)
    wav_path=ROOT/"audio"/f"act{act+1}_tmp.wav"
    with wave.open(str(wav_path),'wb') as wf:
        wf.setnchannels(2); wf.setsampwidth(2); wf.setframerate(sr); wf.writeframes(pcm.tobytes())
    ogg_path=ROOT/"audio"/f"act{act+1}.ogg"
    subprocess.run(["ffmpeg","-y","-loglevel","error","-i",str(wav_path),"-c:a","libvorbis","-b:a","128k",str(ogg_path)],check=True)
    wav_path.unlink()
    print("created",ogg_path.name,round(ogg_path.stat().st_size/1024/1024,2),"MiB")

# Short original one-shot sound effects.
def write_sfx(filename, kind, length, basefreq):
    n=int(sr*length); tt=np.arange(n,dtype=np.float32)/sr
    if kind=="noise":
        sig=rng.normal(0,1,n).astype(np.float32)*np.exp(-tt*17.0)
    elif kind=="zap":
        f=basefreq*(1.0+3.0*np.exp(-tt*22.0))
        phase=2*np.pi*np.cumsum(f)/sr
        sig=np.sin(phase)*np.exp(-tt*10.0)
    elif kind=="bass":
        f=basefreq*(1.0-0.68*np.minimum(tt/max(length,0.001),1.0))
        phase=2*np.pi*np.cumsum(f)/sr
        sig=np.sin(phase)*np.exp(-tt*8.0)
    else:
        sig=(np.sin(2*np.pi*basefreq*tt)+0.4*np.sin(2*np.pi*basefreq*2.01*tt))*np.exp(-tt*7.0)
    sig=np.clip(sig*0.5,-0.95,0.95)
    stereo=np.stack([sig,sig],axis=1)
    with wave.open(str(ROOT/"audio"/filename),'wb') as wf:
        wf.setnchannels(2); wf.setsampwidth(2); wf.setframerate(sr); wf.writeframes((stereo*32767).astype(np.int16).tobytes())

write_sfx("shot.wav","zap",0.12,920)
write_sfx("pickup.wav","tone",0.32,660)
write_sfx("dash.wav","noise",0.24,0)
write_sfx("emp.wav","bass",0.6,100)
write_sfx("hurt.wav","noise",0.16,0)
write_sfx("boss_down.wav","bass",1.0,80)
print("assets total",round(sum(p.stat().st_size for p in ROOT.rglob('*') if p.is_file())/1024/1024,2),"MiB")
