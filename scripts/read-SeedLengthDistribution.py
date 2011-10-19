#!/usr/bin/python

import sys

file=sys.argv[1]

sum=0
n=0
for line in open(file):
	tokens=line.split()
	count=int(tokens[1])
	sum += int(tokens[0]) * count
	n+= count
	

print sum
print n
