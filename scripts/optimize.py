#!/usr/bin/python

#this is a prototype for coverage detection 2.0

import sys
import math
import os

# numerical integration from i to j, substract the triangle
def score(x,y,i,j):
	k=i+1
	if k==len(x):
		return 0
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
	yval=math.log(int(t[1]))
	xval=int(t[0])
	x.append(xval)
	y.append(yval)
f.close()

smoothx=[]
smoothy=[]
i=0
while i<len(x):
	yval=y[i]*2
	xval=x[i]
	n=2.0
	if i<len(x)-1:
		yval+=y[i+1]
		n+=1
	elif i>0:
		yval+=y[i-1]
		n+=1
	yval/=n
	smoothx.append(xval)
	smoothy.append(yval)
	i+=1

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
		if smoothy[i+j]<smoothy[i+j+1] and smoothy[i+j+1] > smoothy[i+j+2]:
			lower+=1
		elif smoothy[i+j]>smoothy[i+j+1] and smoothy[i+j+1] < smoothy[i+j+2]:
			higher+=1
		j+=1
	j=0

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

max=y[bestI]
i=bestI
maxI=i
while i<=bestJ:
	if y[i]>max:
		maxI=i
		max=y[i]
	i+=1

def dotProduct(u,v):
	return u[0]*v[0]+u[1]*v[1]

def distance(qx,qy,px,py,rx,ry):
	pq=[qx-px,qy-py]
	pr=[rx-px,ry-py]
	return math.sqrt(dotProduct(pq,pq)-dotProduct(pq,pr)**2/(0.0+dotProduct(pr,pr)))

# this is not good, but we will find a way
if maxI==bestI:
	i=bestI
	bestDistance=0
	maxI=i
	while i<=bestJ:
		d=distance(x[i],y[i],x[bestI],y[bestI],x[bestJ],y[bestJ])
		if d>bestDistance:
			bestDistance=d
			maxI=i
		i+=1
	
print "Peak is: "+str(x[maxI])+","+str(y[maxI])

os.system("echo 'r=read.table(\""+file+"\")\n\
pdf(\""+file+".pdf\")\n\
plot(r[[1]],log(r[[2]]),type=\"l\",col=\"black\",main=\"Coverage distribution\",xlab=\"Coverage\",ylab=\"Density\")\n\
lines(c("+str(x[bestI])+","+str(x[bestJ])+"),c("+str(y[bestI])+","+str(y[bestJ])+"),col=\"red\")\n\
points(c("+str(x[bestI])+","+str(x[bestJ])+","+str(x[maxI])+"),c("+str(y[bestI])+","+str(y[bestJ])+","+str(y[maxI])+"),col=\"red\")\n\
dev.off()'|R --vanilla &>/dev/null")
print "Wrote "+file+".pdf"

