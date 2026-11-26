import sys,glob,datetime as dt
from zoneinfo import ZoneInfo
from PIL import Image,ImageDraw,ImageFont
A=ZoneInfo("Europe/Amsterdam");U=dt.timezone.utc
src=sys.argv[1]; out=sys.argv[2]; step=int(sys.argv[3]); cols=int(sys.argv[4])
ph=sorted(glob.glob(src+"/photos/*/*.jpg"))[::step]
W,H=400,300
rows=(len(ph)+cols-1)//cols
sheet=Image.new("RGB",(cols*W,rows*(H+28)),"white");d=ImageDraw.Draw(sheet)
try: font=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",18)
except: font=ImageFont.load_default()
for i,p in enumerate(ph):
    day=p.split("/")[-2]; t=p.split("/")[-1][:6]
    utc=dt.datetime.strptime(day+t,"%Y-%m-%d%H%M%S").replace(tzinfo=U)
    im=Image.open(p).resize((W,H))
    x=(i%cols)*W;y=(i//cols)*(H+28)
    sheet.paste(im,(x,y+28)); d.text((x+6,y+4),utc.astimezone(A).strftime("%a %H:%M"),fill="black",font=font)
sheet.save(out,quality=85); print(len(ph),sheet.size)
