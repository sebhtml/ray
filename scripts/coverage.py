#!/usr/bin/python
f=open("Ray-CoverageDistribution.txt")
x=[]
y=[]
for l in f:
	if l[0]=='#':
		continue
	t=l.split()
	x.append(int(t[0]))
	y.append(int(t[1]))
f.close()
i=0
print "y"
while i<len(x):
	print str(x[i])+" "+str(y[i])
	i+=1
i=0
print "dy/dx"
while i<len(x)-1:
	dy=y[i+1]-y[i]
	dx=x[i+1]-x[i]
	dydx=dy/dx
	print str(x[i])+" "+str(dydx)
	i+=1

