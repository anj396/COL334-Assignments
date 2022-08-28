import matplotlib.pyplot as plt

import numpy as np
import sys

import csv


filename=sys.argv[1]

x=[]
y=[]

with open(filename) as f:
    lines = f.readlines()
    for i in lines:
        
        a,b,c=i.split()
        
    
        x.append(a)
        y.append(b)
        




fig=plt.gcf()
fig.set_size_inches(16,12)

la =sys.argv[3]
plt.plot(x,y,label = la)
plt.xlabel("Time(in seconds)")
plt.ylabel("Congestion Window")
plt.title('Congestion Window vs Time') 
plt.legend()

#plt.show()

 

name=sys.argv[2]
fig.savefig(name,dpi=100)
#plt.show()

