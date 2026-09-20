#!/usr/bin/env python3
import math
import os
import random
import struct
import sys
import zlib

W, H = 760, 430

def blank():
    return bytearray(W * H * 4)

def blend(buf, x, y, rgba):
    if x < 0 or y < 0 or x >= W or y >= H:
        return
    r,g,b,a = rgba
    if a <= 0:
        return
    i=(y*W+x)*4
    da=buf[i+3]
    sa=a/255.0
    dda=da/255.0
    oa=sa + dda*(1.0-sa)
    if oa <= 0:
        return
    for c,sv in enumerate((r,g,b)):
        dv=buf[i+c]
        buf[i+c]=max(0,min(255,round((sv*sa + dv*dda*(1-sa))/oa)))
    buf[i+3]=max(0,min(255,round(oa*255)))

def line(buf, x0,y0,x1,y1,rgba,width=1,broken=False,rng=None):
    dx=x1-x0; dy=y1-y0
    steps=max(1,int(max(abs(dx),abs(dy))))
    rng=rng or random
    for s in range(steps+1):
        if broken and rng.random()<0.18:
            continue
        t=s/steps
        x=round(x0+dx*t); y=round(y0+dy*t)
        for yy in range(y-width//2,y+width//2+1):
            for xx in range(x-width//2,x+width//2+1):
                blend(buf,xx,yy,rgba)

def blob(buf,cx,cy,rx,ry,rgba,seed):
    rng=random.Random(seed)
    jitter=[rng.uniform(0.72,1.18) for _ in range(32)]
    x0=max(0,int(cx-rx-2)); x1=min(W,int(cx+rx+3))
    y0=max(0,int(cy-ry-2)); y1=min(H,int(cy+ry+3))
    for y in range(y0,y1):
        for x in range(x0,x1):
            dx=(x-cx)/max(1.0,rx); dy=(y-cy)/max(1.0,ry)
            d=math.sqrt(dx*dx+dy*dy)
            ang=(math.atan2(dy,dx)+math.pi)/(2*math.pi)
            j=jitter[int(ang*31.999)]
            edge=j
            if d<edge:
                fade=max(0.0,min(1.0,(edge-d)/0.22))
                a=round(rgba[3]*fade)
                blend(buf,x,y,(rgba[0],rgba[1],rgba[2],a))

def soft_ellipse(buf,cx,cy,rx,ry,rgba):
    x0=max(0,int(cx-rx*2)); x1=min(W,int(cx+rx*2)+1)
    y0=max(0,int(cy-ry*2)); y1=min(H,int(cy+ry*2)+1)
    for y in range(y0,y1):
        for x in range(x0,x1):
            dx=(x-cx)/max(1.0,rx); dy=(y-cy)/max(1.0,ry)
            d2=dx*dx+dy*dy
            if d2>4.0: continue
            a=round(rgba[3]*math.exp(-1.55*d2))
            if a:
                blend(buf,x,y,(rgba[0],rgba[1],rgba[2],a))

def png_chunk(tag,data):
    return struct.pack(">I",len(data))+tag+data+struct.pack(">I",zlib.crc32(tag+data)&0xffffffff)

def write_png(path,buf):
    raw=bytearray()
    stride=W*4
    for y in range(H):
        raw.append(0)
        raw.extend(buf[y*stride:(y+1)*stride])
    payload=(b"\x89PNG\r\n\x1a\n"+
             png_chunk(b"IHDR",struct.pack(">IIBBBBB",W,H,8,6,0,0,0))+
             png_chunk(b"IDAT",zlib.compress(bytes(raw),9))+
             png_chunk(b"IEND",b""))
    with open(path,"wb") as f:
        f.write(payload)

def mask_wear_to_panel_interiors(buf, panels, inset=6, radius=7):
    # Keep wear strictly inside the blue module faces so the blue border remains clean.
    def inside_round_rect(x,y,l,t,r,b,rad):
        if x < l or x >= r or y < t or y >= b:
            return False
        if l+rad <= x < r-rad or t+rad <= y < b-rad:
            return True
        cx = l+rad if x < l+rad else r-rad-1
        cy = t+rad if y < t+rad else b-rad-1
        dx=x-cx; dy=y-cy
        return dx*dx+dy*dy <= rad*rad

    for y in range(H):
        for x in range(W):
            keep=False
            for l,t,r,b in panels:
                il, it, ir, ib = l+inset, t+inset, r-inset, b-inset
                if inside_round_rect(x,y,il,it,ir,ib,radius):
                    keep=True
                    break
            if not keep:
                i=(y*W+x)*4
                buf[i+3]=0
    return buf

def generate_wear():
    buf=blank()
    panels=[(16,70,278,414),(288,70,618,414),(628,70,744,414)]
    rng=random.Random(12501)
    for pi,(l,t,r,b) in enumerate(panels):
        strength=1.25 if pi==1 else 1.0
        # irregular chipped edge clusters
        for n in range(34 if pi==1 else 24):
            side=rng.randrange(4)
            if side==0:
                cx=rng.uniform(l+5,r-5); cy=t+rng.uniform(-1,4)
                rx=rng.uniform(2,9); ry=rng.uniform(1,4)
            elif side==1:
                cx=rng.uniform(l+5,r-5); cy=b+rng.uniform(-4,1)
                rx=rng.uniform(2,9); ry=rng.uniform(1,4)
            elif side==2:
                cx=l+rng.uniform(-1,4); cy=rng.uniform(t+5,b-5)
                rx=rng.uniform(1,4); ry=rng.uniform(2,9)
            else:
                cx=r+rng.uniform(-4,1); cy=rng.uniform(t+5,b-5)
                rx=rng.uniform(1,4); ry=rng.uniform(2,9)
            color=(190,198,201,round(rng.uniform(34,75)*strength))
            if rng.random()<0.38:
                color=(126,72,40,round(rng.uniform(34,67)*strength))
            blob(buf,cx,cy,rx,ry,color,12500+pi*100+n)
        # tiny interior wear flecks
        for n in range(24 if pi==1 else 14):
            cx=rng.uniform(l+10,r-10); cy=rng.uniform(t+14,b-14)
            rx=rng.uniform(0.7,2.2); ry=rng.uniform(0.6,1.8)
            col=(150,154,151,rng.randint(18,42))
            if rng.random()<0.28:
                col=(133,76,43,rng.randint(20,48))
            blob(buf,cx,cy,rx,ry,col,13000+pi*100+n)
        # a few fine, broken scratches, intentionally short
        for n in range(7 if pi==1 else 4):
            x=rng.uniform(l+12,r-38); y=rng.uniform(t+20,b-20)
            ln=rng.uniform(13,42); slope=rng.uniform(-0.18,0.18)
            alpha=rng.randint(22,48)
            line(buf,x,y,x+ln,y+slope*ln,(214,220,222,alpha),1,True,rng)
    return mask_wear_to_panel_interiors(buf, panels, inset=6, radius=7)

def generate_glass():
    buf=blank()
    # broad soft reflection bands; calculated per pixel so there are no hard vector edges
    bands=[(88,0.34,22,10),(545,0.17,15,6)]
    for y in range(H):
        for x in range(W):
            a=0.0
            for center,slope,sigma,maxa in bands:
                d=x-(center+slope*y)
                a+=maxa*math.exp(-(d*d)/(2*sigma*sigma))
            if a>0.35:
                blend(buf,x,y,(238,245,249,min(18,round(a))))
    rng=random.Random(12502)
    # broad matte wipe traces / fingerprint-like haze
    for n in range(17):
        soft_ellipse(buf,rng.uniform(25,W-25),rng.uniform(22,H-22),
                     rng.uniform(22,72),rng.uniform(7,25),
                     (226,235,240,rng.randint(2,6)))
    for n in range(5):
        soft_ellipse(buf,rng.uniform(40,W-40),rng.uniform(25,H-25),
                     rng.uniform(18,45),rng.uniform(8,20),
                     (7,12,16,rng.randint(2,4)))
    # very fine cover scratches, sparse and broken
    for n in range(13):
        x=rng.uniform(25,W-95); y=rng.uniform(18,H-18)
        ln=rng.uniform(20,70); slope=rng.uniform(-0.16,0.16)
        line(buf,x,y,x+ln,y+slope*ln,(242,247,250,rng.randint(8,18)),1,True,rng)
    # subtle dusty edge haze
    for n in range(70):
        side=rng.randrange(4)
        if side<2:
            x=rng.uniform(0,W); y=rng.uniform(0,9) if side==0 else rng.uniform(H-9,H)
        else:
            x=rng.uniform(0,9) if side==2 else rng.uniform(W-9,W); y=rng.uniform(0,H)
        blob(buf,x,y,rng.uniform(0.7,2.3),rng.uniform(0.7,2.3),
             (220,228,232,rng.randint(3,10)),15000+n)
    return buf

def main():
    out=sys.argv[1] if len(sys.argv)>1 else "."
    os.makedirs(out,exist_ok=True)
    write_png(os.path.join(out,"ugly_wear_overlay.png"),generate_wear())
    write_png(os.path.join(out,"ugly_glass_overlay.png"),generate_glass())
    print("Generated 760x430 RGBA GUI overlays")

if __name__=="__main__":
    main()
