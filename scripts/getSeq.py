#!/usr/bin/python
"""Get a sequence from a fasta file by specifying the sequence name

Usage:
    getSeq.py fasta_file sequence_name
"""

# Importing modules
import sys

if len(sys.argv)!=3:
	print __doc__
	sys.exit(1)

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
