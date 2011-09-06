#!/usr/bin/python


import sys


file="selfMap.psl"

# 2022    0       0       0       0       0       0       0       +       contig-0        2022    0       2022    contig-0        2022    0       2022    1       2022,   0,      0,
i=0

for line in open(file):
	i+=1

	if i<6:
		continue

	tokens=line.split()
	matches=int(tokens[0])
	query=tokens[9]
	target=tokens[13]
	queryLength=int(tokens[10])
	targetLength=int(tokens[14])
	if query==target:
		continue
	ratio=matches/(0.0+queryLength)

	if ratio < 0.5:
		continue

	if queryLength < 5000:
		continue

	print line.strip()


