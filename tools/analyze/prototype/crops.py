import sys,glob,datetime as dt
from zoneinfo import ZoneInfo
from PIL import Image,ImageDraw,ImageFont,ImageOps
A=ZoneInfo("Europe/Amsterdam");U=dt.timezone.utc
src,out,step=sys.argv[1],sys.argv[2],int(sys.argv[3]); box=tuple(map(int,sys.argv[4].split(",")))
ph=sorted(glob.glob(src+"/photos/*/*.jpg"))[::step]
font=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",16)
cw,ch=box[2]-box[0],box[3]-box[1]; cols=4; rows=(len(ph)+cols-1)//cols
S=Image.new("RGB",(cols*cw,rows*(ch+24)),"white");d=ImageDraw.Draw(S)
for i,p in enumerate(ph):
    day=p.split("/")[-2]; t=p.split("/")[-1][:6]
    utc=dt.datetime.strptime(day+t,"%Y-%m-%d%H%M%S").replace(tzinfo=U)
    im=ImageOps.autocontrast(Image.open(p).crop(box))
    x=(i%cols)*cw;y=(i//cols)*(ch+24); S.paste(im,(x,y+24)); d.text((x+4,y+3),utc.astimezone(A).strftime("%a %H:%M"),fill="black",font=font)
S.save(out,quality=88);print(S.size)
