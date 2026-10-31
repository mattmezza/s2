"""Render documentation screenshots on private Xvfb with a synthetic capture.
Requires Pillow, ImageMagick, Xvfb and xdotool. No desktop or real code is captured.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import time
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]

def render(binary):
    destination=ROOT/'docs/screenshots';destination.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='s2-docshots-') as tmp:
        directory=Path(tmp);fixture=directory/'capture.png'
        image=Image.new('RGB',(900,400),'#101116');draw=ImageDraw.Draw(image)
        draw.rounded_rectangle((0,0,899,399),radius=10,outline='#343743',width=1)
        draw.rounded_rectangle((1,1,898,35),radius=9,fill='#22242d')
        draw.rectangle((1,22,898,35),fill='#22242d')
        ui=ImageFont.truetype(str(ROOT/'assets/fonts/Geist-Regular.ttf'),13)
        mono_path=subprocess.check_output(['fc-match','-f','%{file}','monospace']).decode()
        mono=ImageFont.truetype(mono_path,15)
        draw.text((18,10),'workspace_cache.go',font=ui,fill='#d9e0ee')
        draw.text((706,10),'●  Go     UTF-8',font=ui,fill='#91a8df')
        lines=[
            'func BuildPreview(ctx context.Context) error {',
            '    capture := screenshot.From(ctx)',
            '    defer capture.Close()',
            '',
            '    buffer := make([]byte, 4 << 20)',
            '    if err := upload.Flush(buffer); err != nil {',
            '        return fmt.Errorf("preview flush: %w", err)',
            '    }',
            '    return nil',
            '}',
        ]
        for i,line in enumerate(lines):
            y=66+i*25
            draw.text((28,y),str(41+i),font=mono,fill='#596074')
            draw.text((80,y),line,font=mono,fill='#a9bce5' if i not in (0,4,6) else '#d0b583')
        draw.line((1,369,898,369),fill='#252832')
        draw.text((16,379),'Git: fix/buffer-ownership',font=ui,fill='#72839e')
        draw.text((704,379),'900 × 400  PNG',font=ui,fill='#72839e')
        image.save(fixture)
        display=directory/'display'
        with display.open('w') as out:
            server=subprocess.Popen(['Xvfb','-displayfd',str(out.fileno()),'-screen','0','1400x1000x24'],pass_fds=(out.fileno(),),stderr=subprocess.DEVNULL)
        app=None
        try:
            while not display.read_text().strip():time.sleep(.03)
            env=dict(os.environ,DISPLAY=':'+display.read_text().strip())
            def x(*args):return subprocess.check_output(['xdotool',*args],env=env).decode().strip()
            app=subprocess.Popen([str(binary),'-i',str(fixture)],env=env)
            for _ in range(100):
                result=subprocess.run(['xdotool','search','--name','^s2$'],env=env,capture_output=True,text=True)
                if result.returncode==0:window=result.stdout.splitlines()[0];break
                time.sleep(.03)
            else:raise RuntimeError('s2 did not open')
            x('windowfocus',window,'windowsize',window,'1200','900');time.sleep(.2)
            x('key','r','f','bracketleft','bracketleft')
            x('mousemove','216','411','mousedown','1','mousemove','1008','439','mouseup','1')
            x('key','Escape','a')
            x('mousemove','930','560','mousedown','1','mousemove','860','435','mouseup','1')
            x('key','e')
            x('mousemove','895','497','mousedown','1','mousemove','970','465','mouseup','1')
            x('key','Escape','6','t','f')
            x('mousemove','760','565','click','1');x('type','Own the buffer.');x('key','Return','Escape','3')
            def shot(name):
                time.sleep(.2)
                subprocess.run(['import','-window',window,str(destination/name)],env=env,check=True)
            shot('workspace.png')
            x('mousemove','1046','862','click','1');shot('about.png')
            x('key','Escape');x('windowsize',window,'640','640');shot('compact.png')
            x('key','q');assert app.wait(timeout=5)==0
        finally:
            if app and app.poll() is None:app.terminate();app.wait()
            server.terminate();server.wait()

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--binary',type=Path,default=ROOT/'s2')
    render(parser.parse_args().binary.resolve())
