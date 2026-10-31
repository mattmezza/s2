"""Check coalesced pen/marker input preserves every point; profile 4K drawing.
Requires Xvfb and xdotool. Does not use the user's display.
"""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import time
import zlib

ROOT=Path(__file__).resolve().parents[1]

def png_fixture(path):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    width,height=3840,2160
    row=b'\0'+bytes((36,42,52))*width
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(row*height,1))+chunk(b'IEND',b''))

def run(binary):
    with tempfile.TemporaryDirectory(prefix='s2-motion-') as tmp:
        directory=Path(tmp)
        display_file=directory/'display'
        with display_file.open('w') as out:
            server=subprocess.Popen(['Xvfb','-displayfd',str(out.fileno()),'-screen','0','1920x1080x24'],pass_fds=(out.fileno(),),stderr=subprocess.DEVNULL)
        app=None
        try:
            for _ in range(100):
                if display_file.read_text().strip():break
                time.sleep(.05)
            env=dict(os.environ,DISPLAY=':'+display_file.read_text().strip(),S2_PROFILE='1')
            fixture=directory/'4k.png';png_fixture(fixture)
            def xdo(*args):
                return subprocess.check_output(['xdotool',*map(str,args)],env=env).decode().strip()
            def launch(tool,output):
                nonlocal app
                app=subprocess.Popen([str(binary),'-i',str(fixture),'-o',str(output)],env=env,stderr=subprocess.PIPE)
                time.sleep(.5)
                window=xdo('search','--name','^s2$').splitlines()[0]
                xdo('windowfocus',window,'key','s' if tool=='cursor' else tool,'mousemove',500,450)
                if tool!='cursor':xdo('mousedown',1)
            for tool in ('p','m','l','r','o','b','x','cursor'):
                outputs=[]
                for paced in (True,False):
                    output=directory/('%s-%s.png'%(tool,paced));launch(tool,output)
                    args=[];start=time.monotonic()
                    for i in range(100):
                        commands=['mousemove',500+i*5,450+(i%10)*8]
                        if paced:
                            xdo(*commands);time.sleep(.003)
                        else:args+=commands
                    if args:xdo(*args)
                    xdo('mouseup',1,'key','ctrl+s')
                    assert app.wait(timeout=30)==0
                    report=app.stderr.read().decode().strip()
                    print(tool,'paced' if paced else 'burst','wall_s=%.3f'%(time.monotonic()-start),report)
                    outputs.append(output.read_bytes())
                assert outputs[0]==outputs[1],tool+': coalescing changed stroke geometry'
            print('PASS: pen, marker, shapes and region-effect exports match paced input exactly')
        finally:
            if app is not None and app.poll() is None:
                app.terminate();app.wait(timeout=5)
            server.terminate();server.wait(timeout=5)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--binary',type=Path,default=ROOT/'s2')
    run(parser.parse_args().binary.resolve())
