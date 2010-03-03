#!/usr/bin/python

import sys
import os

if len(sys.argv)<3:
	print "Usage: "+sys.argv[0]+" reference contigs "
	sys.exit()
reference=sys.argv[1]
contigs=sys.argv[2]

blat="/home/boiseb01/blat"

# compute alignments if necessary.
os.system("if test -f $(md5sum "+contigs+"|awk '{print $1}');then echo 0>/dev/null; else "+blat+" "+reference+" "+contigs+" $(md5sum "+contigs+"|awk '{print $1}') -fastMap -out=blast;fi")

correct=0
perfect=0
totalBases=0
minimumContigSize=500

f=open("tmp")
currentQuery=""
firstIdentityDone=False
n=0
lengthIsParsed=False
currentIdentities=0
theLength=0

for i in f:
	if i.find("Query=")==0:
		#if currentQuery!="":
			
		currentQuery=i.split()[1].strip()
		lengthIsParsed=False
		firstIdentityDone=False
	elif not lengthIsParsed and currentQuery!="":
		lengthIsParsed=True
		theLength=int(i.split()[0].replace("(",""))
	elif i.find("Identities =")==1:
		if not firstIdentityDone:
			currentIdentities=int(i.split()[2].split("/")[0])
			firstIdentityDone=True
			#print currentQuery+" Length="+str(theLength)+" Matches="+str(currentIdentities)
			if theLength<minimumContigSize:
				continue
			if theLength==currentIdentities:
				perfect+=1
			totalBases+=theLength
			n+=1

print "Statistics for >="+str(minimumContigSize)+"-nt contigs"
print "TotalBases="+str(totalBases)
print "Perfect="+str(perfect)+"/"+str(n)
meanLength=totalBases/n
print "MeanLength="+str(meanLength)
