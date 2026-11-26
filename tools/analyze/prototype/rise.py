import glob,csv,datetime as dt,sys
from zoneinfo import ZoneInfo
from PIL import Image
import numpy as np
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt, matplotlib.dates as md
A=ZoneInfo("Europe/Amsterdam");U=dt.timezone.utc
R=sys.argv[1]; PX_PER_CM=60.0
T,back,front=[],[],[]
for p in sorted(glob.glob(R+"/photos/*/*.jpg"))[1:]:
    day=p.split("/")[-2]; t=p.split("/")[-1][:6]
    lt=dt.datetime.strptime(day+t,"%Y-%m-%d%H%M%S").replace(tzinfo=U).astimezone(A)
    g=np.asarray(Image.open(p).convert("L"),dtype=float)
    col=np.convolve(g[300:480,420:460].mean(axis=1),np.ones(7)/7,mode="same"); d=np.diff(col)[5:-5]; b=300+5+int(np.argmax(np.abs(d)))
    col=np.convolve(g[420:540,500:600].mean(axis=1),np.ones(5)/5,mode="same"); d=np.diff(col)[4:-4]; f=420+4+int(np.argmax(d))
    T.append(lt);back.append(b);front.append(f)
back=np.array(back,dtype=float);front=np.array(front,dtype=float)
from scipy.signal import medfilt
back=medfilt(back,3);back[0]=back[1] if abs(back[0]-back[1])>15 else back[0]
rb=(back[0]-back)/PX_PER_CM; rf=(front[0]-front)/PX_PER_CM
rows=[(int(a),float(b2),float(c)) for a,b2,c in csv.reader(open(R+"/data/2026-11-12.csv")) if a and int(a)>=1794472544]
X=[dt.datetime.fromtimestamp(e,A) for e,_,_ in rows]
fig,ax=plt.subplots(3,1,figsize=(11,7.5),sharex=True,gridspec_kw={"height_ratios":[1.3,1,1]})
ax[0].plot(T,rb,"o-",ms=3,color="#534AB7",label="achterrand oppervlak")
ax[0].plot(T,rf,"s-",ms=3,color="#1D9E75",label="voorrand oppervlak")
ax[0].set_ylabel("Stijging t.o.v. start (cm)"); ax[0].legend(frameon=False,loc="upper left"); ax[0].set_ylim(-0.1,1.0)
ax[1].plot(X,[r[1] for r in rows],color="#D85A30",lw=1.2); ax[1].set_ylabel("Temperatuur (°C)")
ax[2].plot(X,[r[2] for r in rows],color="#378ADD",lw=1.2); ax[2].set_ylabel("Luchtvochtigheid (%)")
for a in ax:
    a.grid(alpha=.25)
    for s in ["top","right"]: a.spines[s].set_visible(False)
ax[2].xaxis.set_major_formatter(md.DateFormatter("%H:%M",tz=A)); ax[2].xaxis.set_major_locator(md.HourLocator(interval=2,tz=A))
fig.suptitle("Proef 2 – 12 november, 1:1:1 in kleine pot, keukenlucht (geschatte stijging uit camerabeelden)",fontsize=11,x=0.01,ha="left")
fig.tight_layout(); fig.savefig("proef2_stijging_en_klimaat.png",dpi=130)
print("eind back %.2f front %.2f"%(rb[-1],rf[-1]))
for i in range(0,len(T),4): print(T[i].strftime("%H:%M"),round(rb[i],2),round(rf[i],2))
