"""Extract usable Runware glyphs as transparent, embedded 48px alpha masks."""
from pathlib import Path
from PIL import Image, ImageChops
root=Path(__file__).resolve().parents[1]
atlas=Image.open(root/'assets/icons/runware-atlas.png').convert('RGB')
names={'pen':3,'select':4,'marker':5,'pixelate':6,'blur':9,'picker':10,'help':11,'save':15}
rows=['/* Generated from the Runware atlas by scripts/prepare_icons.py. */']
for name,index in names.items():
 x,y=index%4*256,index//4*256
 tile=atlas.crop((x,y,x+256,y+256)).convert('L')
 tile=tile.point(lambda v: max(0,min(255,(v-48)*255//180)))
 box=tile.getbbox();tile=tile.crop(box)
 tile.thumbnail((36,36),Image.Resampling.LANCZOS)
 alpha=Image.new('L',(48,48));alpha.paste(tile,((48-tile.width)//2,(48-tile.height)//2))
 rgba=Image.new('RGBA',(48,48),'white');rgba.putalpha(alpha);rgba.save(root/f'assets/icons/{name}.png')
 rows.append(f'static const unsigned char icon_{name}[48*48] = {{'+','.join(map(str,alpha.tobytes()))+'};')
(root/'assets/icons/masks.h').write_text('\n'.join(rows)+'\n')
