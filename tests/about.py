"""About/help modal and link integration on private Xvfb; never opens a browser."""
import os
from pathlib import Path
import subprocess
import tempfile
import time

ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='s2-about-') as tmp:
    directory=Path(tmp);display=directory/'display';link_log=directory/'links'
    opener=directory/'xdg-open'
    opener.write_text('#!/bin/sh\nprintf "%s\\n" "$1" >> "$S2_TEST_REPO_LOG"\n')
    opener.chmod(0o755)
    with display.open('w') as out:
        server=subprocess.Popen(['Xvfb','-displayfd',str(out.fileno()),'-screen','0','1200x900x24'],pass_fds=(out.fileno(),),stderr=subprocess.DEVNULL)
    app=None
    try:
        while not display.read_text().strip():time.sleep(.03)
        env=dict(os.environ,DISPLAY=':'+display.read_text().strip(),PATH=tmp+os.pathsep+os.environ['PATH'],S2_TEST_REPO_LOG=str(link_log))
        def x(*args):return subprocess.check_output(['xdotool',*args],env=env).decode().strip()
        def launch(output):
            global app
            app=subprocess.Popen([str(ROOT/'s2'),'-i',str(ROOT/'cover-og.png'),'-o',str(output)],env=env)
            time.sleep(.4)
            window=x('search','--name','^s2$').splitlines()[0]
            x('windowfocus',window,'windowsize',window,'1000','800');time.sleep(.15)
            return window
        baseline=directory/'baseline.png';launch(baseline);x('key','ctrl+s');assert app.wait(timeout=5)==0
        output=directory/'about.png';window=launch(output)
        def show_about():x('mousemove','946','762','click','1');time.sleep(.1)
        show_about()
        # Key events cannot change tools, save, or exit behind the panel.
        x('key','r','ctrl+s','Delete');assert app.poll() is None and not output.exists()
        # About is 560x500 centered at 220,150. Link opens only the stub.
        x('mousemove','500','477','click','1')
        for _ in range(100):
            if link_log.exists():break
            time.sleep(.02)
        assert link_log.read_text().splitlines()==['https://github.com/mattmezza/s2']
        x('key','Escape');assert app.poll() is None
        show_about();x('key','Tab','Return')
        time.sleep(.15)
        assert len(link_log.read_text().splitlines())==2
        x('key','Escape');show_about();x('mousemove','230','140','click','1') # backdrop
        show_about();x('mousemove','731','191','click','1') # close button
        # New help icon beside info opens a separate, dismissible panel.
        x('mousemove','910','762','click','1');x('key','Escape')
        # Compact footer still exposes both controls and never paints into export.
        x('windowsize',window,'400','420');time.sleep(.1)
        x('mousemove','356','338','click','1');time.sleep(.1)
        x('key','Escape')
        x('key','ctrl+s');assert app.wait(timeout=5)==0
        assert output.read_bytes()==baseline.read_bytes(),'About/help changed capture pixels'
        print('PASS: responsive About/help, modal isolation, real link dispatch, keyboard navigation and clean export')
    finally:
        if app and app.poll() is None:app.terminate();app.wait()
        server.terminate();server.wait()
