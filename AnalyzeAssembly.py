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
os.system("if test -f $(md5sum "+contigs+"|awk '{print $1}');then echo 0>/dev/null; else "+blat+" "+reference+" "+contigs+" $(md5sum "+contigs+"|awk '{print $1}') -fastMap -out=blast>/dev/null;fi;md5sum "+contigs+"|awk '{print $1}'>tmp")

correct=0
perfect=0
totalBases=0
minimumContigSize=500

f=open("tmp")
fileName=""
for i in f:
	fileName=i.split()[0].strip()
	break
f.close
f=open(fileName)
currentQuery=""
firstIdentityDone=False
n=0
lengthIsParsed=False
currentIdentities=0
theLength=0
perfectAlignment=0
coolAt999=0
coolAt99=0
coolAt97=0
others=0
firstPositionFound=False
secondScoreStarted=False
positions=[]
firstPosition=0
lastPosition=0
appended=False
print "Alignments not perfect:"
for i in f:
	if i.find("Query=")==0:
		if currentQuery!="":
			if theLength>=minimumContigSize:
				positions.append([firstPosition,lastPosition])
		currentQuery=i.split()[1].strip()
		lengthIsParsed=False
		firstIdentityDone=False
		firstPositionFound=False
		secondScoreStarted=False
		appended=False
	elif not lengthIsParsed and currentQuery!="":
		lengthIsParsed=True
		theLength=int(i.split()[0].replace("(",""))
	elif i.find("Identities =")==1:
		if not firstIdentityDone:
			currentIdentities=int(i.split()[2].split("/")[0])
			alignmentLength=int(i.split()[2].split("/")[1])
			firstIdentityDone=True
			#print currentQuery+" Length="+str(theLength)+" Matches="+str(currentIdentities)
			if theLength<minimumContigSize:
				continue
			if theLength==currentIdentities:
				perfect+=1
			elif currentIdentities>=0.999*theLength	:
				coolAt999+=1
			elif currentIdentities>=0.99*theLength:
				coolAt99+=1
			elif currentIdentities>=0.97*theLength:
				coolAt97+=1
			else:
				others+=1

			if theLength!=currentIdentities:
				print str(theLength-currentIdentities)+" mismatches, length="+str(theLength)+" alignmentLength="+str(alignmentLength)
			totalBases+=theLength
			n+=1
	elif firstIdentityDone and i.find(" Score")==0:
		secondScoreStarted=True
	elif i.find("Sbjct:")==0:
		if not firstPositionFound:
			firstPosition=int(i.split()[1])
			firstPositionFound=True
		if not secondScoreStarted:
			lastPosition=int(i.split()[3])


positions.append([firstPosition,lastPosition])



print ""
print "Statistics for >="+str(minimumContigSize)+"-nt contigs"
print "TotalBases="+str(totalBases)
print "Perfect="+str(perfect)+"/"+str(n)
print "AtLeast99.9%="+str(coolAt999)+"/"+str(n)
print "AtLeast99.00%="+str(coolAt99)+"/"+str(n)
print "AtLeast97.00%="+str(coolAt97)+"/"+str(n)
print "Others="+str(others)
meanLength=totalBases/n
print "MeanLength="+str(meanLength)

