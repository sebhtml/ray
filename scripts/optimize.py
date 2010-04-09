#!/usr/bin/python

import sys
import math

# numerical integration from i to j, substract the triangle
def score(x,y,i,j):
	k=i+1
	min=y[i]
	if y[j]<min:
		min=y[j]
	while k<j:
		if y[k]<min:
			return 0
		k+=1
	whole=0
	k=i
	while k<j:
		whole+=(y[k+1]+y[k])*(x[k+1]-x[k])/2.0
		k+=1
	toRemove=(y[i]+y[j])*(x[j]-x[i])/2.0
	s=whole-toRemove
	#print str(x[i])+"-"+str(x[j])+":"+str(s)
	return s

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

bestI=0
bestJ=0
bestScore=0
i=0

theMax=255
while i<len(x)-30:
	lower=0
	higher=0
	j=0
	while j<30-2:
		if y[i+j]<y[i+j+1] and y[i+j+1] > y[i+j+2]:
			lower+=1
		elif y[i+j]>y[i+j+1] and y[i+j+1] < y[i+j+2]:
			higher+=1
		j+=1
	if lower>7 and higher>7:
		theMax=i
		break
	i+=1
i=0
while i<theMax:
	j=i+1
	while j<theMax:
		s=score(x,y,i,j)
		if s>bestScore:
			bestI=i
			bestJ=j
			bestScore=s
		j+=1
	i+=1

print "Best score: "+str(bestScore)+" from "+str(x[bestI])+","+str(y[bestI])+" to "+str(x[bestJ])+","+str(y[bestJ])
