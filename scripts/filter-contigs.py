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

p=1
def writeToFile(head,body,output,threshold,p):
	if len(body)>=threshold:
		output.write(">"+str(head)+"\n")
		i=0
		columns=80
		while i<len(body):
			output.write(body[i:(i+columns)]+"\n")
			i+=columns


for line in input:
	if line[0]=='>':
		if head!="":
			writeToFile(head,body,output,threshold,p)
			p+=1
		head=line.strip().replace(">","").split()[0].strip()
		body=""
	else:
		body+=line.strip()

writeToFile(head,body,output,threshold,p)

output.close()
input.close()
