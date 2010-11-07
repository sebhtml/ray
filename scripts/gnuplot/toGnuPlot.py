#!/usr/bin/python

import math

k=15
while k<=32:
	file="Ecoli-k="+str(k)+".fasta-TheCoverageDistribution.tab"
	for line in open(file):
		if line[0]=="#":
			continue
		tokens=line.split()
		if tokens[0]=="255":
			continue
		print str(k)+"\t"+tokens[0]+"\t"+str(math.log(int(tokens[1])))
	print ""
	k+=1
