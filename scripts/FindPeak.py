
import sys
import math

x=[]
y=[]
for i in open(sys.argv[1]):
	l=i.split()
	x.append(int(l[0]))
	y.append(int(l[1]))

i=0
derivatives=[]

while i<len(x)-1:
	xi=x[i]
	yi=y[i]
	xi1=x[i+1]
	yi1=y[i+1]
	derivative=(yi1-yi)/(0.0+xi1-xi)
	derivatives.append(derivative)
	i+=1

i=0
maxScore=0
best=0
while i<len(derivatives)-1:
	xi=x[i]
	yi=y[i]
	leftScore=1
	j=i-1
	step=256
	min=i-step
	if min<0:
		min=0
	max=i+step
	if max>len(derivatives)-1:
		max=len(derivatives)-1
	o=1
	while j>=min:
		val=derivatives[j]
		if val>0:
			o+=1
		else:
			o=1
		leftScore+=o*o
		j-=1
	j=i+1
	rightScore=1
	o=1
	while j<max:
		val=derivatives[j]
		if val<0:
			o+=1
		else:
			o=1
		rightScore+=o*o
		j+=1
	score=leftScore*rightScore
	if score>maxScore:
		maxScore=score
		best=xi
	print str(xi)+"\t"+str(yi)+"\t"+str(score)
	i+=1

print best
