"""Native connector editing and automatic selection on a private Xvfb display."""
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import time
import zlib

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='s2-connectors-') as tmp:
    directory=Path(tmp)
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    fixture=directory/'input.png'
    fixture.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',600,400,8,6,0,0,0))+chunk(b'IDAT',zlib.compress((b'\0'+bytes((30,32,38,255))*600)*400))+chunk(b'IEND',b''))
    display=directory/'display'
    with display.open('w') as out:
        server=subprocess.Popen(['Xvfb','-displayfd',str(out.fileno()),'-screen','0','1200x900x24'],pass_fds=(out.fileno(),),stderr=subprocess.DEVNULL)
    app=None
    try:
        while not display.read_text().strip():time.sleep(.03)
        env=dict(os.environ,DISPLAY=':'+display.read_text().strip())
        def x(*args):return subprocess.check_output(['xdotool',*args],env=env).decode().strip()
        def launch(name):
            global app
            output=directory/(name+'.png')
            app=subprocess.Popen([str(ROOT/'s2'),'-i',str(fixture),'-o',str(output)],env=env)
            time.sleep(.25)
            window=x('search','--name','^s2$').splitlines()[0]
            x('windowfocus',window,'windowsize',window,'1000','800')
            time.sleep(.1)
            return output,window
        def finish(output):
            x('key','ctrl+s');assert app.wait(timeout=10)==0
            return output.read_bytes()
        output,_=launch('baseline');baseline=finish(output)
        for tool in ('a','l','r','o','h','b','x','p','m','n'):
            output,_=launch('auto-select-'+tool)
            x('key',tool)
            x('mousemove','300','350','mousedown','1','mousemove','700','450','mouseup','1')
            x('key','Delete')
            assert finish(output)==baseline, tool+' did not auto-select after placement'
        # Selection edits must affect exported pixels, and picker targets must
        # agree with the equivalent palette shortcuts.
        def selected_case(name,tool,keys=(),picker=None,before=()):
            output,window=launch(name)
            if before:x('key',*before)
            x('key',tool)
            if tool=='t':x('mousemove','300','350','click','1')
            else:x('mousemove','300','350','mousedown','1','mousemove','700','450','mouseup','1')
            if tool=='t':x('type','Gradient text');x('key','Return')
            if keys:x('key',*keys)
            if picker:
                scale=976/1016
                bx=round(12+(640-108)*scale);by=round(16+32*scale)
                if picker=='fill':x('keydown','Shift_L')
                x('mousemove',str(bx),str(by),'click','1')
                if picker=='fill':x('keyup','Shift_L')
                time.sleep(.08)
                # The first swatch is white.
                px=12+516*scale-110;py=16+64*scale+10
                if picker=='hex':
                    x('mousemove',str(round(px+100)),str(round(py+226)),'click','1')
                    x('type','1e2026');x('key','Return')
                elif picker=='sample':
                    x('mousemove',str(round(px+100)),str(round(py+268)),'click','1')
                    x('mousemove','250','250','click','1')
                else:
                    if picker=='fill-tab':x('mousemove',str(round(px+190)),str(round(py+22)),'click','1')
                    x('mousemove',str(round(px+40)),str(round(py+60)),'click','1')
            return finish(output)
        arrow=selected_case('arrow-original','a')
        assert selected_case('arrow-cycle-back','a',('e','E'))==arrow
        assert selected_case('arrow-size','a',('bracketright',))!=arrow
        assert selected_case('arrow-color','a',('1',))!=arrow
        assert selected_case('arrow-no-fill','a',('at',))==arrow
        rect=selected_case('rect-original','r')
        assert selected_case('rect-size','r',('bracketright',))!=rect
        assert selected_case('rect-stroke','r',('1',))!=rect
        assert selected_case('rect-fill','r',('at',))!=rect
        assert selected_case('rect-white-stroke-key','r',('6',))==selected_case('rect-white-stroke-picker','r',picker='stroke')
        assert selected_case('rect-white-fill-key','r',('asciicircum',))==selected_case('rect-white-fill-picker','r',picker='fill')
        assert selected_case('rect-fill-tab','r',picker='fill-tab')==selected_case('rect-white-fill-key2','r',('asciicircum',))
        assert selected_case('rect-hex','r',picker='hex')==selected_case('rect-sampled','r',picker='sample')
        # Escape clears the selection, so Delete afterwards must retain the shape.
        assert selected_case('rect-escape','r',('Escape','Delete'))==rect
        for tool in ('r','o','n','h','t'):
            default=selected_case('gradient-default-'+tool,tool)
            flat=selected_case('gradient-off-'+tool,tool,('g',))
            assert flat!=default,tool+' gradient did not switch off'
            assert selected_case('gradient-restored-'+tool,tool,('g','g'))==default
            assert selected_case('gradient-global-'+tool,tool,before=('g',))==flat
        for tool in ('r','o'):
            outlined=selected_case('outline-default-'+tool,tool)
            no_outline=selected_case('outline-off-'+tool,tool,('F',))
            assert no_outline!=outlined
            assert selected_case('outline-restored-'+tool,tool,('F','F'))==outlined
            assert selected_case('outline-global-'+tool,tool,before=('F',))==no_outline
        versions=[]
        for style in range(6):
            output,window=launch('edge-'+str(style))
            x('key','a');x('mousemove','300','350','mousedown','1','mousemove','700','350','mouseup','1')
            if style:x('key',*(['e']*style))
            x('key','less') # enable matching start end
            x('mousemove','500','350','mousedown','1','mousemove','500','430','mouseup','1')
            if style==1:
                time.sleep(.15)
                subprocess.run(['import','-window',window,'/tmp/s2-curved-arrow.png'],env=env,check=True)
            versions.append(finish(output))
        assert len(set(versions))==6,'end styles must produce distinct rendered pixels'
        output,_=launch('straight')
        x('key','l');x('mousemove','300','350','mousedown','1','mousemove','700','350','mouseup','1')
        straight=finish(output)
        output,_=launch('curved-line')
        x('key','l');x('mousemove','300','350','mousedown','1','mousemove','700','350','mouseup','1')
        x('mousemove','500','350','mousedown','1','mousemove','500','430','mouseup','1')
        assert finish(output)!=straight,'dragging midpoint must bend line'
        output,_=launch('straightened-line')
        x('key','l');x('mousemove','300','350','mousedown','1','mousemove','700','350','mouseup','1')
        x('mousemove','500','350','mousedown','1','mousemove','500','430','mouseup','1')
        x('mousemove','500','430','mousedown','1','keydown','Shift_L','mousemove','500','400','mouseup','1','keyup','Shift_L')
        assert finish(output)==straight,'Shift dragging must straighten line'
        print('PASS: automatic selection, six curved arrow styles, curved lines and straightening')
    finally:
        if app and app.poll() is None:app.terminate();app.wait()
        server.terminate();server.wait()
