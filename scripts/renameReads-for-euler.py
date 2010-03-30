#!/usr/bin/python

# a script to rename reads so EULER-SR can use them.


#@ERR005143.1 ID49_20708_20H04AAXX_R1:7:1:41:356/1
#AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
#+
#hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh

# transform into:

#@ID49_20708_20H04AAXX_R1:7:1:41:356/1
#AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
#+
#hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh


#@SRR001666.1 071112_SLXA-EAS1_s_7:5:1:817:345 length=36
#GGGTGATGGCCGCTGCCGATGGCGTCAAATCCCACC
#+SRR001666.1 071112_SLXA-EAS1_s_7:5:1:817:345 length=36
#IIIIIIIIIIIIIIIIIIIIIIIIIIIIII9IG9IC

# into

#@071112_SLXA-EAS1_s_7:5:1:817:345/1
#GGGTGATGGCCGCTGCCGATGGCGTCAAATCCCACC
#+
#IIIIIIIIIIIIIIIIIIIIIIIIIIIIII9IG9IC


import os
import sys

if len(sys.argv)!=3:
	print "usage"
	print "renameReads.py <string> <fastaFile>"
	print "This script takes a fasta file <fastaFile> and add <string> at the end of all read names"
	sys.exit()

string=sys.argv[1]

f=open(sys.argv[2])
while 1:
	line=f.readline()
	if line=="":
		break
	if line[0]=='@':
		tokens=line.strip().split(" ")
		print "@"+tokens[1].strip()+string
		print f.readline().strip()
		f.readline()
		print '+'
		print f.readline().strip()

f.close()
