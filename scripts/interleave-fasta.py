#!/usr/bin/python
# author: Sébastien Boisvert
# part of Ray distribution
# this script takes two fasta files and interleave them.

import sys

if len(sys.argv)==1:
	print "Provide 2 fasta files..."
	sys.exit()

f1=open(sys.argv[1])
f2=open(sys.argv[2])

while True:
	line=f1.readline()
	if len(line)==0:
		break
	print line.strip()
	print f1.readline().strip()
	print f2.readline().strip()
	print f2.readline().strip()

f2.close()
f1.close()
