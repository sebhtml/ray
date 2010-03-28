#!/usr/bin/python

import sys

if len(sys.argv)!=4:
	print "usage:"
	print "filter-contigs.py fastaFile.fa minimumLength output.fa"
	exit()

input=open(sys.argv[1])
threshold=int(sys.argv[2])
output=open(sys.argv[3],"w+")

head=""
body=""

for line in input:
	if line[0]=='>':
		if head!="":
			if len(body)>=threshold:
				output.write(head+"\n"+body+"\n")
		head=line.strip()
		body=""
	else:
		body+=line.strip()


if len(body)>=threshold:
	output.write(head+"\n"+body+"\n")

output.close()
input.close()
