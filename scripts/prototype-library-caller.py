#!/usr/bin/python
#encoding: utf-8

# analyze a PREFIX.LibraryX.txt file
# this is a prototype.
# Author: Sébastien Boisvert

import sys

file=sys.argv[1]

print ""
print file

x=[]
y=[]

for line in open(file):
	tokens=line.split()
	x.append(int(tokens[0]))
	y.append(int(tokens[1]))

minI=0
maxI=len(x)-1

minimumCount=5

while minI<len(x) and y[minI]<minimumCount:
	minI+=1

while maxI>=0 and y[maxI]<minimumCount:
	maxI-=1

minX=x[minI]
maxX=x[maxI]
middle=(maxX-minX)/2

i=0
peakLeft=i
while i<len(x) and x[i]<middle:
	if y[i]>y[peakLeft]:
		peakLeft=i
	i+=1

peakRight=i
while i<len(x) and x[i]<=maxX:
	if y[i]>y[peakRight]:
		peakRight=i
	i+=1

def callPeak(x,y,peak,step):
	middle=(x[len(x)-1]-x[0])/2
	first=x[peak]-step
	last=x[peak]+step

	if x[peak]>middle and first<middle:
		first=middle

	if x[peak]<middle and last>middle:
		last=middle

	i=0
	sum=0
	n=0
	thresold=0.001
	dataPoints=0
	while i<len(x):
		if x[i]>=first and x[i] <= last and y[i]> y[peak]*thresold:
			sum+=x[i]*y[i]
			n+=y[i]
			dataPoints+=1
		i+=1
	average=sum/n

	minimumDatapoints=2
	if dataPoints<minimumDatapoints:
		return

	sum=0
	i=0
	while i<len(x):
		if x[i]>=first and x[i] <= last and y[i]> y[peak]*thresold:
			diff=(x[i]-average)
			sum+=diff*diff*y[i]
		i+=1
	
	standardDeviation=((sum+0.0)/n)**(0.5)

	first=average-standardDeviation
	last=average+standardDeviation
	i=0
	busy=0
	while i<len(y):
		if x[i]>=first and x[i] <= last:
			busy+=1
		i+=1

	occupancy=(busy+0.0)/(last-first+1)*100

	threshold=20
	if occupancy<threshold:
		return

	print "Peak Average= "+str(average)+" StandardDeviation= "+str(standardDeviation)+" Count= "+str(n)+" Points= "+str(dataPoints)+" Quality= "+str(occupancy)+" %"

step=1000
#print "Peak1 "+str(x[peakLeft])
#print "Peak2 "+str(x[peakRight])
if x[peakRight]<x[peakLeft]+step:
	#print "1 peak"
	peak=peakLeft
	if y[peakRight]>y[peakLeft]:
		peak=peakRight
	callPeak(x,y,peak,step)
else:
	#print "2 peak"
	callPeak(x,y,peakLeft,step)
	callPeak(x,y,peakRight,step)
