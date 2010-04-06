#!/usr/bin/python

import sys

if len(sys.argv)!=3:
	print "usage:"
	print "filter-contigs.py fastaFile.fa output.fa"
	exit()

input=open(sys.argv[1])
output=open(sys.argv[2],"w+")

head=""
body=""
threshold=0
p=1
def writeToFile(head,body,output,threshold,p):
	if len(body)>=threshold:
		output.write(head+"\n")
		i=0
		columns=80
		while i<len(body):
			output.write(body[i:(i+columns)])
			i+=columns
	output.write("\n")


for line in input:
	if line[0]=='>':
		if head!="":
			writeToFile(head,body,output,threshold,p)
			p+=1
		head=line.strip()
		body=""
	else:
		body+=line.strip()

writeToFile(head,body,output,threshold,p)

output.close()
input.close()
