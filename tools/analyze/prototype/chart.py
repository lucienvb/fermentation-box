import csv,glob,datetime as dt,sys
from zoneinfo import ZoneInfo
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt, matplotlib.dates as md
A=ZoneInfo("Europe/Amsterdam"); src=sys.argv[1]
rows=sorted((int(a),float(b),float(c)) for p in glob.glob(src+"/data/2026-*.csv") for a,b,c in csv.reader(open(p)))
# break lines at gaps
X,T,H=[],[],[]
prev=None
for e,t,h in rows:
    if prev and e-prev>90: X.append(dt.datetime.fromtimestamp(prev+60,A));T.append(None);H.append(None)
    X.append(dt.datetime.fromtimestamp(e,A));T.append(t);H.append(h);prev=e
import numpy as np
Xn=[x if x else np.nan for x in X]
fig,ax=plt.subplots(2,1,figsize=(11,5.5),sharex=True)
ax[0].plot(X,[t if t is not None else np.nan for t in T],color="#D85A30",lw=1.3)
ax[0].set_ylabel("Temperatuur (°C)"); ax[0].set_ylim(17,20.5)
ax[0].axhspan(24,28,color="#EF9F27",alpha=.15)
ax[1].plot(X,[h if h is not None else np.nan for h in H],color="#378ADD",lw=1.3)
ax[1].set_ylabel("Luchtvochtigheid (%)"); ax[1].set_ylim(40,56)
for a in ax:
    a.grid(alpha=.25); a.axvspan(dt.datetime(2026,11,11,17,20,tzinfo=A),dt.datetime(2026,11,11,19,6,tzinfo=A),color="#888",alpha=.18)
    a.axvspan(dt.datetime(2026,11,11,19,47,tzinfo=A),dt.datetime(2026,11,11,20,4,tzinfo=A),color="#888",alpha=.18)
    for s in ["top","right"]: a.spines[s].set_visible(False)
ax[0].text(dt.datetime(2026,11,11,17,25,tzinfo=A),20.2,"geen metingen",fontsize=9,color="#555")
ax[0].annotate("± 20:00 opwarming keuken",xy=(dt.datetime(2026,11,11,20,10,tzinfo=A),19.45),xytext=(dt.datetime(2026,11,11,21,30,tzinfo=A),20.1),fontsize=9,arrowprops=dict(arrowstyle="-",color="#555"))
ax[1].xaxis.set_major_formatter(md.DateFormatter("%a %H:%M",tz=A)); ax[1].xaxis.set_major_locator(md.HourLocator(interval=3,tz=A))
fig.suptitle("Proef 1 – keukenlucht, 11–12 november (lokale tijd)",fontsize=12,x=0.01,ha="left")
fig.tight_layout(); fig.savefig("metingen_proef1.png",dpi=130); print("ok")
