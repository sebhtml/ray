#!/usr/bin/python

import sys

file=sys.argv[1]
name=sys.argv[2]

dump=False

for line in open(file):
	if line[0]=='>':
		if line.find(name)>=0:
			dump=True
		elif dump:
			break
	if dump:
		print line.strip()
