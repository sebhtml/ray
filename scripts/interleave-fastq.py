#!/usr/bin/python

# interleave two fastq files.

import sys
file1=sys.argv[1]
file2=sys.argv[2]
f1=open(file1)
f2=open(file2)

while True:
	line1a=f1.readline()
	if len(line1a)==0:
		break
	line1b=f1.readline()
	f1.readline()
	f1.readline()
	line2a=f2.readline()
	line2b=f2.readline()
	f2.readline()
	f2.readline()
	print line1a.strip()
	print line1b.strip()
	print line2a.strip()
	print line2b.strip()

f2.close()
f1.close()
