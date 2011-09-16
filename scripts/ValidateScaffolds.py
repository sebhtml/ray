#!/usr/bin/python

import sys

if len(sys.argv)!=2:
	print "Usage: "
	print "ValidateScaffolds.py Prefix"
	sys.exit()

prefix=sys.argv[1]

map=prefix+"/Contigs.fasta.500.mums_prefix.coords"
scaffolds=prefix+"/ScaffoldComponents.txt"

correctLocations={}
coverages={}
identities={}

lineNumber=0
for line in open(map):
	if lineNumber>=5:
		tokens=line.split()
		start=int(tokens[0])
		contigStart=int(tokens[3])
		contigEnd=int(tokens[4])
		strand='F'
		if contigStart>contigEnd:
			strand='R'
		queryCoverage=float(tokens[15])
		identity=float(tokens[9])
		contigName=tokens[18].strip()
		chromosomeName=tokens[17].strip()
		if contigName not in coverages or queryCoverage>coverages[contigName] or (queryCoverage==coverages[contigName] and identity> identities[contigName]):
			coverages[contigName]=queryCoverage
			correctLocations[contigName]=[chromosomeName,start,strand]
			identities[contigName]=identity

	lineNumber+=1
	
incorrectScaffolds=0

order=[]
lastScaffold=""

def isScaffoldCorrect(scaffold,order,correctLocations):
	chromosomes={}
	correctOrder=[]
	found=0

	if len(order)==1:
		return True

	for contig in order:
		name=contig[0]

		if name not in correctLocations:
			return True

		entry=correctLocations[name]
		chromosome=entry[0]
		start=entry[1]
		strand=entry[2]
		correctOrder.append((name,start,strand))
		if chromosome not in chromosomes:
			chromosomes[chromosome]=0
		chromosomes[chromosome]+=1
	
	# more than one chromosome
	if len(chromosomes.keys())!=1:
		return False

	correctOrder=sorted(correctOrder,key=lambda item: item[1])

	firstCorrect=correctOrder[0][0]
	if firstCorrect==order[len(order)-1][0]:
		reverse=[]
		i=len(order)-1
		while i>=0:
			newStrand="F"
			contig=order[i][0]
			strand=order[i][1]
			if strand==newStrand:
				newStrand="R"
			reverse.append([contig,newStrand])
			i-=1
		order=reverse

	# validate order and observedOrder

	print ""
	print scaffold
	print "Observed: "+str(order)
	print "Correct: "+str(correctOrder)
	if len(order)!=len(correctOrder):
		print "Incorrect"
		return False

	i=0
	while i<len(order):
		if order[i][0]!=correctOrder[i][0] or  order[i][1]!=correctOrder[i][2]:
			print "Incorrect"
			return False
		i+=1
	return True

for line in open(scaffolds):
	tokens=line.split()
	if len(tokens)==0:
		continue
	scaffold=tokens[0].strip()
	contig=tokens[1].strip()
	strand=tokens[2].strip()
	if contig=="gap":
		continue
	if lastScaffold=="":
		lastScaffold=scaffold
		order.append([contig,strand])
	elif lastScaffold==scaffold:
		order.append([contig,strand])
	else:
		if not isScaffoldCorrect(lastScaffold,order,correctLocations):
			incorrectScaffolds+=1
		order=[]
		lastScaffold=scaffold
		order.append([contig,strand])
	
print incorrectScaffolds
