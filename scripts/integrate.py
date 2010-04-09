#!/usr/bin/python

import sys
import math
import os

file=sys.argv[1]
f=open(file)
x=[]
y=[]
for l in f:
	if l[0]=='#':
		continue
	t=l.split()
	x.append(int(t[0]))
	y.append(math.log(int(t[1])))
f.close()

def integral(x,y,i,j):
	whole=0
	k=i
	while k<j:
		whole+=(y[k+1]+y[k])*(x[k+1]-x[k])/2.0
		k+=1
	return whole

i=0
xint=[]
yint=[]
while i<len(x)-1:
	s=integral(x,y,0,i)
	xint.append(x[i])
	yint.append(s)
	i+=1

i=0
while i<len(xint)-1:
	dy=yint[i+1]-yint[i]
	dx=xint[i+1]-xint[i]
	dydx=dy/(dx+0.0)
	print str(xint[i])+" "+str(dydx)
	i+=1
