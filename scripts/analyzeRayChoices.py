#!/usr/bin/python

import sys

hasChoice=False
choiceCoverages=[]
choices=0
position=0

for line in open(sys.argv[1]):
	if line.find("CurrentVertex=")>=0:
		if len(line.split())<2:
			print line

		position=int(line.split()[1].replace("@",""))
		hasChoice=False
	elif line.find("choices")>=0:
		hasChoice=True
		choiceCoverages=[]
		choices=int(line.split()[0])
	elif line.find("Coverage=")>=0:
		coverageValue=int(line.replace("Coverage=",""))
		choiceCoverages.append(coverageValue)
		if len(choiceCoverages)==choices:
			i=0
			lowerThan10=0
			while i<len(choiceCoverages):
				if choiceCoverages[i]<10:
					lowerThan10+=1
				i+=1
			
			if (len(choiceCoverages)-lowerThan10) == 1:
				continue

			print "Position "+str(position)+" "+str(choices)
			i=0

			while i<len(choiceCoverages):
				print " "+str(choiceCoverages[i])
				i+=1
