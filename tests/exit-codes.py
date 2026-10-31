"""Check successful exits and genuine failures on an isolated X server."""
import ctypes
import ctypes.util
import os
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='s2-exit-') as tmp:
    display = Path(tmp) / 'display'
    with display.open('w') as out:
        server = subprocess.Popen(['Xvfb', '-displayfd', str(out.fileno()), '-screen', '0', '1200x900x24'], pass_fds=(out.fileno(),), stderr=subprocess.DEVNULL)
    try:
        for _ in range(100):
            if display.read_text().strip():
                break
            time.sleep(.03)
        env = dict(os.environ, DISPLAY=':' + display.read_text().strip())
        for key, flags, expected in (
            ('Escape', ['--modal'], 0),
            ('Escape', ['--modal', '-c'], 0),
            ('WM_DELETE_WINDOW', [], 0),
            ('regular-Escape', [], 0),
            ('regular-Enter', [], 0),
            ('modal-Enter', ['--modal'], 0),
            ('ctrl+s', ['-D', tmp], 0),
            ('ctrl+c', [], 0),
            ('ctrl+s', ['-o', tmp + '/missing/output.png'], 1),
        ):
            output = Path(tmp) / 'cancelled.png'
            app = subprocess.Popen([str(ROOT / 's2'), '-i', str(ROOT / 'cover-og.png'), *(['-o', str(output)] if '-D' not in flags else []), *flags], env=env, stderr=subprocess.PIPE if key in ('WM_DELETE_WINDOW','regular-Escape','regular-Enter','modal-Enter') else None)
            try:
                for _ in range(100):
                    result = subprocess.run(['xdotool', 'search', '--name', '^s2$'], env=env, capture_output=True, text=True)
                    if result.returncode == 0:
                        window = result.stdout.splitlines()[0]
                        break
                    time.sleep(.03)
                else:
                    raise AssertionError('window did not open')
                if key in ('regular-Escape','regular-Enter','modal-Enter'):
                    subprocess.run(['xdotool','windowfocus','--sync',window,'key','Escape' if key=='regular-Escape' else 'Return'],env=env,check=True)
                    time.sleep(.15)
                    assert app.poll() is None,'Esc/Enter unexpectedly quit'
                    assert not output.exists()
                    key='WM_DELETE_WINDOW'
                if key == 'WM_DELETE_WINDOW':
                    protocols=subprocess.check_output(['xprop','-id',window,'WM_PROTOCOLS'],env=env).decode()
                    assert 'WM_DELETE_WINDOW' in protocols
                    # Send the same ICCCM message as mwm; no WM runs in Xvfb.
                    class Data(ctypes.Union):
                        _fields_=[('l',ctypes.c_long*5)]
                    class Client(ctypes.Structure):
                        _fields_=[('type',ctypes.c_int),('serial',ctypes.c_ulong),('send_event',ctypes.c_int),('display',ctypes.c_void_p),('window',ctypes.c_ulong),('message_type',ctypes.c_ulong),('format',ctypes.c_int),('data',Data)]
                    class Event(ctypes.Union):
                        _fields_=[('client',Client),('pad',ctypes.c_long*24)]
                    lib=ctypes.CDLL(ctypes.util.find_library('X11'))
                    lib.XOpenDisplay.argtypes=[ctypes.c_char_p];lib.XOpenDisplay.restype=ctypes.c_void_p
                    lib.XInternAtom.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_int];lib.XInternAtom.restype=ctypes.c_ulong
                    lib.XSendEvent.argtypes=[ctypes.c_void_p,ctypes.c_ulong,ctypes.c_int,ctypes.c_long,ctypes.POINTER(Event)]
                    lib.XCloseDisplay.argtypes=[ctypes.c_void_p]
                    dpy=lib.XOpenDisplay(env['DISPLAY'].encode())
                    try:
                        event=Event();event.client.type=33;event.client.window=int(window)
                        event.client.message_type=lib.XInternAtom(dpy,b'WM_PROTOCOLS',0);event.client.format=32
                        event.client.data.l[0]=lib.XInternAtom(dpy,b'WM_DELETE_WINDOW',0)
                        assert lib.XSendEvent(dpy,int(window),0,0,ctypes.byref(event))
                    finally:lib.XCloseDisplay(dpy)
                else:
                    subprocess.run(['xdotool', 'windowfocus', '--sync', window, 'key', '--clearmodifiers', key], env=env, check=True)
                code = app.wait(timeout=10)
                assert code == expected, (key, flags, code, expected)
                if key=='WM_DELETE_WINDOW':
                    assert app.stderr.read()==b'', 'WM close emitted an error'
                if key in ('Escape', 'WM_DELETE_WINDOW'):
                    assert not output.exists(), 'cancellation wrote an output'
                if key == 'ctrl+s' and expected==0:
                    assert len(list(Path(tmp).glob('*.png'))) == 1
                print('PASS:', key, flags, 'exit', code)
            finally:
                if app.poll() is None:
                    app.terminate()
                    app.wait()
    finally:
        server.terminate()
        server.wait()
