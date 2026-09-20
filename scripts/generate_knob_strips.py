#!/usr/bin/env python3
import math
import os
import struct
import sys
import zlib

def paeth(a,b,c):
    p=a+b-c
    pa=abs(p-a); pb=abs(p-b); pc=abs(p-c)
    return a if pa<=pb and pa<=pc else (b if pb<=pc else c)

def read_png(path):
    data=open(path,"rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("Not a PNG")
    off=8
    ids=[]
    w=h=None
    bd=ct=inter=None
    while off < len(data):
        ln=struct.unpack(">I",data[off:off+4])[0]
        typ=data[off+4:off+8]
        chunk=data[off+8:off+8+ln]
        off += 12 + ln
        if typ == b"IHDR":
            w,h,bd,ct,comp,filt,inter=struct.unpack(">IIBBBBB",chunk)
        elif typ == b"IDAT":
            ids.append(chunk)
        elif typ == b"IEND":
            break
    if (bd,ct,inter)!=(8,6,0):
        raise ValueError(f"Expected non-interlaced RGBA8 PNG, got bit_depth={bd}, color_type={ct}, interlace={inter}")
    raw=zlib.decompress(b"".join(ids))
    bpp=4
    stride=w*bpp
    out=bytearray(w*h*bpp)
    prev=bytearray(stride)
    pos=0
    for y in range(h):
        ft=raw[pos]; pos+=1
        cur=bytearray(raw[pos:pos+stride]); pos+=stride
        for x in range(stride):
            a=cur[x-bpp] if x>=bpp else 0
            b=prev[x]
            c=prev[x-bpp] if x>=bpp else 0
            if ft==1: cur[x]=(cur[x]+a)&255
            elif ft==2: cur[x]=(cur[x]+b)&255
            elif ft==3: cur[x]=(cur[x]+((a+b)//2))&255
            elif ft==4: cur[x]=(cur[x]+paeth(a,b,c))&255
            elif ft!=0: raise ValueError(f"Unsupported PNG filter {ft}")
        out[y*stride:(y+1)*stride]=cur
        prev=cur
    return w,h,out

def resize_frame(src,sw,sh,dw,dh,src_y):
    out=bytearray(dw*dh*4)
    for y in range(dh):
        fy=(y+0.5)*sh/dh-0.5
        ybase=math.floor(fy)
        y0=max(0,min(sh-1,int(ybase)))
        y1=min(sh-1,y0+1)
        wy=fy-ybase
        for x in range(dw):
            fx=(x+0.5)*sw/dw-0.5
            xbase=math.floor(fx)
            x0=max(0,min(sw-1,int(xbase)))
            x1=min(sw-1,x0+1)
            wx=fx-xbase
            for ch in range(4):
                p00=src[((src_y+y0)*sw+x0)*4+ch]
                p10=src[((src_y+y0)*sw+x1)*4+ch]
                p01=src[((src_y+y1)*sw+x0)*4+ch]
                p11=src[((src_y+y1)*sw+x1)*4+ch]
                v=(p00*(1-wx)+p10*wx)*(1-wy)+(p01*(1-wx)+p11*wx)*wy
                out[(y*dw+x)*4+ch]=max(0,min(255,round(v)))
    return out

def png_chunk(tag,data):
    return struct.pack(">I",len(data))+tag+data+struct.pack(">I",zlib.crc32(tag+data)&0xffffffff)

def write_png(path,w,h,pix):
    raw=bytearray()
    stride=w*4
    for y in range(h):
        raw.append(0)
        raw.extend(pix[y*stride:(y+1)*stride])
    payload=(b"\x89PNG\r\n\x1a\n"+
             png_chunk(b"IHDR",struct.pack(">IIBBBBB",w,h,8,6,0,0,0))+
             png_chunk(b"IDAT",zlib.compress(bytes(raw),9))+
             png_chunk(b"IEND",b""))
    with open(path,"wb") as f:
        f.write(payload)

def make(master,out_dir):
    sw,sh,src=read_png(master)
    if (sw,sh)!=(125,12625):
        raise ValueError(f"Unexpected master strip size {sw}x{sh}; expected 125x12625")
    frames=101
    os.makedirs(out_dir,exist_ok=True)
    for logical in (54,56,58):
        for scale in (1,2):
            d=logical*scale
            strip=bytearray(d*d*frames*4)
            for frame in range(frames):
                fr=resize_frame(src,125,125,d,d,frame*125)
                rowbytes=d*4
                for y in range(d):
                    dst=((frame*d+y)*d)*4
                    strip[dst:dst+rowbytes]=fr[y*rowbytes:(y+1)*rowbytes]
            suffix="" if scale==1 else "#2.0x"
            path=os.path.join(out_dir,f"ugly_knob_{logical}{suffix}.png")
            write_png(path,d,d*frames,strip)
            print(f"Generated {os.path.basename(path)}: {d}x{d*frames}")

if __name__=="__main__":
    if len(sys.argv)!=3:
        raise SystemExit("usage: generate_knob_strips.py <master.png> <output-dir>")
    make(sys.argv[1],sys.argv[2])
