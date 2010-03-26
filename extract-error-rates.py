#!/usr/bin/python

import os
import sys

if len(sys.argv)!=3:
	print "usage"
	print "extract-error-rates.py reference.fasta assembly.fasta"
	sys.exit()

reference=sys.argv[1]
draft=sys.argv[2]

print "blasting..."
os.system("formatdb -p F -i "+reference)
os.system("blastall -d "+reference+" -i "+draft+"  -e 0.001 -p blastn -F F -o tmp ")

blastFile="tmp"
f=open(blastFile)

mismatches=0
gaps=0

line=f.readline()
while line!="":
	line=f.readline()
	while line!="":
		if line[0:6]=='Query=':
			break
		line=f.readline()
	#print line
	while line!="":
		if line[1:13]=='Identities =':
			break
		line=f.readline()
			
	if line=="":
		break
	#print line
	tokens=line.strip().split(" ")
	identities=tokens[3-1].strip()
	integers=identities.split("/")
	localMismatches=int(integers[1])-int(integers[0])
	if len(tokens)==8:
		gapsStr=tokens[7-1].strip()
		integers=gapsStr.split("/")
		localGaps=int(integers[0])
		gaps+=localGaps
		#print "gaps "+str(localGaps)
	#print str(localMismatches)
	mismatches+=localMismatches

print "totalMismatches="+str(mismatches-gaps)
print "totalGaps="+str(gaps)
f.close()
