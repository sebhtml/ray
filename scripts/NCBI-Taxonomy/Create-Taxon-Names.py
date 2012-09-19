#!/usr/bin/env python
# encoding: UTF-8
# author: Sébastien Boisvert
# date: 2012-09-19

import sys

print("[Create-Taxon-Names.py]")

if len(sys.argv)!=4:
	print("Error: invalid number of arguments.")
	sys.exit(1)


# columns are separated with |
# column 1 is the identifier
# column 3 is the rank
nodeFile=sys.argv[1]

# columns are separated with |
# column 1 is the identifier
# column 2 is the name
# column 4 must contain 'scientific name'
nameFile=sys.argv[2]

outputFile=sys.argv[3]

outputStream=open(outputFile,"w")

storage={}

stream=open(nodeFile,"r")

print("Reading "+nodeFile)

for line in stream:
	columns=line.split("|")
	identifier=int(columns[1-1].strip())
	taxonomicRank=columns[3-1].strip()

	if identifier not in storage:
		storage[identifier]=[]
		storage[identifier].append(taxonomicRank)
print("Done.")

stream.close()

stream=open(nameFile,"r")

validName='scientific name'

print("Reading "+nameFile)

for line in stream:
	columns=line.split("|")
	identifier=int(columns[1-1].strip())
	name=columns[2-1].strip()
	entryType=columns[4-1].strip()

	if entryType!=validName:
		continue

	if identifier not in storage:
		print("Warning: "+name+" is in "+nameFile+" but not in "+nodeFile)
		continue

	if len(storage[identifier])==2:
		print("Warning: "+name+" has many scientific names in "+nameFile)
		continue

	storage[identifier].append(name)

stream.close()
print("Done.")


identifiers=storage.keys()
identifiers.sort()

rankIndex=0
nameIndex=1

for identifier in identifiers:
	name=storage[identifier][nameIndex]
	taxonomicRank=storage[identifier][rankIndex]

	outputStream.write(str(identifier)+"	"+name+"	"+taxonomicRank+"\n")


outputStream.close()

print("Created "+outputFile)
