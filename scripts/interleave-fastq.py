#!/usr/bin/python
# author: Sébastien Boisvert
# part of Ray distribution
# this script takes two fastq files and interleave them.


import sys

if len(sys.argv)==1:
	print "Provide 2 fastq files..."
	sys.exit()

f1=open(sys.argv[1])
f2=open(sys.argv[2])

while True:
	line=f1.readline()
	if len(line)==0:
		break
	print line.strip()
	i=0
	while i<3:
		print f1.readline().strip()
		i+=1
	
	i=0
	while i<4:
		print f2.readline().strip()
		i+=1

f2.close()
f1.close()
