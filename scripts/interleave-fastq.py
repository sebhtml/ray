#!/usr/bin/python

# interleave two fastq files.

import sys
file1=sys.argv[1]
file2=sys.argv[2]
f1=open(file1)
f2=open(file2)

while True:
	line=f1.readline()
	if len(line)==0:
		break
	print line
	i=0
	while i<3:
		print f1.readline()
	
	i=0
	while i<4:
		print f2.readline()

f2.close()
f1.close()
