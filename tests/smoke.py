"""Native integration smoke test; isolated invisible Xvfb, no desktop access."""
import os
import shutil
import re
import ctypes
import ctypes.util
import struct
import zlib
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]

def run():
    with tempfile.TemporaryDirectory(prefix='s2-smoke-') as tmp:
        display_file = Path(tmp) / 'display'
        with display_file.open('w') as out:
            server = subprocess.Popen(['Xvfb', '-displayfd', str(out.fileno()), '-screen', '0', '1200x900x24'], pass_fds=(out.fileno(),), stderr=subprocess.DEVNULL)
        app = None
        try:
            for _ in range(100):
                if display_file.read_text().strip():
                    break
                time.sleep(.05)
            env = dict(os.environ, DISPLAY=':' + display_file.read_text().strip())
            def xdo(*args):
                return subprocess.check_output(['xdotool', *args], env=env).decode().strip()
            def launch(*args, input_path=ROOT / 'cover-og.png'):
                nonlocal app
                app = subprocess.Popen([str(ROOT / 's2'), '-i', str(input_path), *args], env=env)
                time.sleep(.7)
                window = xdo('search', '--name', '^s2').splitlines()[0]
                xdo('windowfocus', window)
                return window
            def top_click(local_x):
                geometry=dict(line.split('=',1) for line in xdo('getwindowgeometry','--shell',xdo('search','--name','^s2$').splitlines()[0]).splitlines() if '=' in line)
                width=int(geometry['WIDTH']);rows=2 if width<900 else 1
                barwidth=520 if rows==2 else 1016
                scale=min(1,(width-24)/barwidth)
                left=(width-int(barwidth*scale))//2
                local=local_x-108;row=0
                if rows==2 and local>=504:local-=504;row=64
                xdo('mousemove',str(int(left+local*scale)),str(int(16+(32+row)*scale)),'click','1')
            # Window mode hints and responsive geometry, without touching the desktop.
            for flags, kind in (((), 'NORMAL'), (('--modal',), 'DIALOG')):
                window=launch(*flags)
                props=subprocess.check_output(['xprop','-id',window,'_NET_WM_WINDOW_TYPE','WM_NORMAL_HINTS'],env=env).decode()
                assert '_NET_WM_WINDOW_TYPE_'+kind in props, props
                if flags:
                    geo=dict(line.split('=',1) for line in xdo('getwindowgeometry','--shell',window).splitlines() if '=' in line)
                    assert abs(int(geo['X'])-(1200-int(geo['WIDTH']))//2)<=1
                    assert abs(int(geo['Y'])-(900-int(geo['HEIGHT']))//2)<=1
                xdo('windowmove',window,'0','0')
                xdo('windowsize',window,'400','420')
                time.sleep(.2)
                top_click(640) # Wrapped color picker.
                xdo('key','Escape')
                top_click(690) # Wrapped size picker.
                xdo('key','Escape')
                xdo('key','q')
                app.wait(timeout=5)
            output = Path(tmp) / 'annotated.png'
            launch('-o', str(output))
            xdo('key', 'r')
            xdo('mousemove', '300', '300', 'mousedown', '1', 'mousemove', '500', '450', 'mouseup', '1')
            xdo('key', 'ctrl+z', 'ctrl+shift+z', 'ctrl+s')
            assert app.wait(timeout=5) == 0
            assert output.read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
            assert output.read_bytes() != (ROOT / 'cover-og.png').read_bytes()
            # Exercise every drawable toolbar tool with real pointer events.
            # Export a baseline through s2 so encoding differences cannot hide a no-op.
            baseline = Path(tmp) / 'baseline.png'
            launch('-o', str(baseline))
            xdo('key', 'question', 'Escape')
            assert app.poll() is None, 'closing help must not quit'
            xdo('key', 'ctrl+s')
            assert app.wait(timeout=5) == 0
            for index in range(1, 12):
                capture = Path(tmp) / ('tool-%d.png' % index)
                launch('-o', str(capture))
                top_click(120+index*38+18)
                xdo('mousemove', '330', '350', 'mousedown', '1', 'mousemove', '490', '430', 'mouseup', '1')
                if index == 7:
                    xdo('type', 'CUI annotation')
                    xdo('key', 'Return')
                xdo('key', 'ctrl+s')
                assert app.wait(timeout=5) == 0
                assert capture.read_bytes() != baseline.read_bytes(), 'tool %d produced no change' % index
            # Theme shortcuts/picker do not change exported capture pixels.
            def window_pixel(window,x,y):
                lib=ctypes.CDLL(ctypes.util.find_library('X11'))
                lib.XOpenDisplay.argtypes=[ctypes.c_char_p];lib.XOpenDisplay.restype=ctypes.c_void_p
                lib.XGetImage.argtypes=[ctypes.c_void_p,ctypes.c_ulong,ctypes.c_int,ctypes.c_int,ctypes.c_uint,ctypes.c_uint,ctypes.c_ulong,ctypes.c_int]
                lib.XGetImage.restype=ctypes.c_void_p
                lib.XGetPixel.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_int];lib.XGetPixel.restype=ctypes.c_ulong
                lib.XDestroyImage.argtypes=[ctypes.c_void_p];lib.XCloseDisplay.argtypes=[ctypes.c_void_p]
                display=lib.XOpenDisplay(env['DISPLAY'].encode())
                image=lib.XGetImage(display,int(window),x,y,1,1,ctypes.c_ulong(-1),2)
                assert image
                pixel=lib.XGetPixel(image,0,0)
                lib.XDestroyImage(image);lib.XCloseDisplay(display)
                return pixel
            themed=Path(tmp)/'themed.png'
            window=launch('-o',str(themed))
            xdo('key','alt+2');time.sleep(.15)
            assert all(((window_pixel(window,10,10)>>shift)&255)>180 for shift in (16,8,0)),'Alt+2 did not apply Light'
            xdo('key','alt+3');time.sleep(.15)
            assert all(((window_pixel(window,10,10)>>shift)&255)<70 for shift in (16,8,0)),'Alt+3 did not apply Tokyo Night'
            xdo('key','alt+9','alt+c')
            xdo('key','Escape','ctrl+s')
            assert app.wait(timeout=5)==0
            assert themed.read_bytes()==baseline.read_bytes(),'theme changed export'
            # Real selection handles: rotation and resizing commit on release.
            variants=[]
            for operation in ('plain','rotate','resize'):
                capture=Path(tmp)/(operation+'.png')
                launch('-o',str(capture))
                xdo('key','r')
                xdo('mousemove','330','350','mousedown','1','mousemove','490','430','mouseup','1')
                if operation!='plain':
                    xdo('key','s')
                    xdo('mousemove','330','350','click','1')
                    if operation=='rotate':
                        xdo('mousemove','410','328','mousedown','1','keydown','Shift_L','mousemove','510','390','mouseup','1','keyup','Shift_L')
                    else:
                        xdo('mousemove','490','430','mousedown','1','keydown','Shift_L','mousemove','650','510','mouseup','1','keyup','Shift_L')
                xdo('key','ctrl+s')
                assert app.wait(timeout=5)==0
                variants.append(capture.read_bytes())
            assert variants[0]!=variants[1], 'rotation handle did not change geometry'
            assert variants[0]!=variants[2], 'resize handle did not change geometry'
            # Selection/delete must return to the original raster.
            deleted = Path(tmp) / 'deleted.png'
            window = launch('-o', str(deleted))
            xdo('key', 'r')
            xdo('mousemove', '330', '350', 'mousedown', '1', 'mousemove', '490', '430', 'mouseup', '1')
            xdo('key', 's')
            xdo('mousemove', '330', '350', 'click', '1', 'key', 'Delete', 'ctrl+s')
            assert app.wait(timeout=5) == 0
            assert deleted.read_bytes() == baseline.read_bytes(), 'selection/delete failed'
            # Narrow and wide resizing preserves the image export.
            resized = Path(tmp) / 'resized.png'
            window = launch('-o', str(resized))
            xdo('mousemove','450','350','keydown','Control_L','click','4','click','4','click','5','mousedown','1','mousemove','600','450','mouseup','1','keyup','Control_L')
            xdo('key', 'plus', 'plus', 'minus', 'equal')
            xdo('windowsize', window, '720', '700')
            time.sleep(.3)
            # Hit areas remain aligned with the scaled toolbar.
            top_click(120+2*38+18)
            xdo('windowsize', window, '1100', '800')
            time.sleep(.3)
            xdo('key', 'ctrl+s')
            assert app.wait(timeout=5) == 0
            assert resized.read_bytes() == baseline.read_bytes(), 'resize altered image'
            launch()
            xdo('key', 'ctrl+y')
            time.sleep(.5)
            clip = subprocess.check_output(['xclip', '-selection', 'clipboard', '-target', 'image/png', '-out'], env=env, timeout=5)
            assert app.poll() is None and clip.startswith(b'\x89PNG'), 'copy without closing failed'
            xdo('key', 'Escape')
            assert app.poll() is None, 'Esc must keep regular windows open'
            xdo('key','q')
            assert app.wait(timeout=5) == 0
            tiny=Path(tmp)/'tiny.png'
            def chunk(kind,data):
                return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
            tiny.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',32,32,8,2,0,0,0))+chunk(b'IDAT',zlib.compress((b'\0'+bytes((40,60,90))*32)*32))+chunk(b'IEND',b''))
            tiny_output=Path(tmp)/'tiny-saved.png'
            window=launch('-o',str(tiny_output),input_path=tiny)
            geometry=dict(line.split('=',1) for line in xdo('getwindowgeometry','--shell',window).splitlines() if '=' in line)
            assert int(geometry['WIDTH'])>=720 and int(geometry['HEIGHT'])>=700
            xdo('key','ctrl+s')
            assert app.wait(timeout=5)==0
            assert struct.unpack('>II',tiny_output.read_bytes()[16:24])==(32,32)
            launch('-D', tmp)
            # Window width is 1168 at this display; centered toolbar origin 94.
            top_click(843)
            assert app.wait(timeout=5) == 0
            timestamped=list(Path(tmp).glob('20??-*.png'))
            assert len(timestamped)==1 and re.fullmatch(r'\d{4}-\d{2}-\d{2}T\d{6}\.\d{6}\.png',timestamped[0].name)
            launch('-D',tmp);xdo('key','ctrl+s');assert app.wait(timeout=5)==0
            assert len(list(Path(tmp).glob('20??-*.png')))==2,'rapid saves overwrote a capture'
            launch()
            top_click(969)
            assert app.wait(timeout=5) == 0
            clip = subprocess.check_output(['xclip', '-selection', 'clipboard', '-target', 'image/png', '-out'], env=env, timeout=5)
            assert clip.startswith(b'\x89PNG\r\n\x1a\n')
            # Selected fill changes and text shortcuts alter existing objects.
            for operation in ('fill','text-plain','text-styled'):
                capture=Path(tmp)/(operation+'.png')
                launch('-o',str(capture))
                if operation=='fill':
                    xdo('key','r');xdo('mousemove','330','350','mousedown','1','mousemove','490','430','mouseup','1')
                    xdo('key','s');xdo('mousemove','330','350','click','1','key','f')
                else:
                    xdo('key','t');xdo('mousemove','350','350','click','1');xdo('type','Styled text');xdo('key','Return','s')
                    xdo('mousemove','365','360','click','1')
                    if operation=='text-styled':xdo('key','shift+b','i','u','s','shift+p','shift+c','shift+3')
                xdo('key','ctrl+s');assert app.wait(timeout=5)==0
            assert (Path(tmp)/'fill.png').read_bytes()!=variants[0],'selected fill had no effect'
            assert (Path(tmp)/'text-plain.png').read_bytes()!=(Path(tmp)/'text-styled.png').read_bytes(),'selected text styling had no effect'
            # T edits in place; Escape restores the original; Enter selects the result.
            for kind in ('original','reference','edited','cancelled'):
                capture=Path(tmp)/('edit-'+kind+'.png')
                launch('-o',str(capture));xdo('key','t');xdo('mousemove','350','350','click','1')
                xdo('type','Edited' if kind=='reference' else 'Original');xdo('key','Return')
                if kind in ('edited','cancelled'):
                    xdo('key','t','ctrl+a');xdo('type','Edited' if kind=='edited' else 'Discard me')
                    xdo('key','Return' if kind=='edited' else 'Escape')
                xdo('key','ctrl+s');assert app.wait(timeout=5)==0
            assert (Path(tmp)/'edit-reference.png').read_bytes()==(Path(tmp)/'edit-edited.png').read_bytes(),'T duplicated or misplaced the edited text'
            assert (Path(tmp)/'edit-original.png').read_bytes()==(Path(tmp)/'edit-cancelled.png').read_bytes(),'Escape changed existing text'
            # Relocated binary with no external fonts/assets and an empty font config.
            standalone=Path(tmp)/'s2-standalone';shutil.copy2(ROOT/'s2',standalone)
            fontconfig=Path(tmp)/'fontconfig.xml'
            fontconfig.write_text('<?xml version="1.0"?><!DOCTYPE fontconfig SYSTEM "fonts.dtd"><fontconfig><cachedir>'+tmp+'/font-cache</cachedir></fontconfig>')
            isolated=dict(env,FONTCONFIG_FILE=str(fontconfig),XDG_CACHE_HOME=tmp)
            standalone_output=Path(tmp)/'standalone.png'
            app=subprocess.Popen([str(standalone),'-i',str(ROOT/'cover-og.png'),'-o',str(standalone_output)],cwd=tmp,env=isolated)
            time.sleep(.8)
            assert app.poll() is None,'relocated binary could not start'
            fds=[str(p.resolve()) for p in Path('/proc/%d/fd'%app.pid).iterdir()]
            assert sum('memfd:s2-geist' in name for name in fds)==2,'fonts were not embedded'
            window=xdo('search','--name','^s2$').splitlines()[0];xdo('windowfocus',window)
            xdo('key','t');xdo('mousemove','350','350','click','1');xdo('type','Embedded Geist');xdo('key','Return','ctrl+s')
            assert app.wait(timeout=5)==0 and standalone_output.read_bytes()!=baseline.read_bytes()
            assert b'SIL OPEN FONT LICENSE' in subprocess.check_output([str(standalone),'--licenses'])
            print('PASS: all drawable toolbar tools, annotate, undo/redo, Ctrl+S save, toolbar Save, toolbar Copy & Close')
        finally:
            if app is not None and app.poll() is None:
                app.terminate()
                app.wait(timeout=5)
            server.terminate()
            server.wait(timeout=5)

if __name__ == '__main__':
    run()
