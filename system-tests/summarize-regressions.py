#!/usr/bin/python

import sys

file=sys.argv[1]

#   & numberOfContigs &scaffolds & bases & meanSize  & n50  & max   & coverage   & misassembledContigs & misassembledScaffolds & mismatches & indels
#  300-strept.sh.Ray & 86 & 68 & 1969888 & 22905 & 44534 &  194158 &  0.9627 & 1 & 1 & 1 & 0 \\

print "<table border=\"1\"><tr><th>Dataset</th><th>Number of contigs</th><th>Number of scaffolds</th><th>Number of bases</th><th>Average contig length</th><th>N50 contig length</th><th>Maximum contig length</th><th>Genome coverage breadth</th><th>Number of misassembled contigs</th><th>Number of incorrect scaffolds</th><th>Nucleotide mismatches</th><th>Nucleotide indels</th></tr>"

for line in open(file):
	if line.find(".Ray")!=-1:
		tokens=line.split("&")
		print "<tr>"
		for i in tokens:
			print "<td>"
			print i.replace('\\','')
			print "</td>"
		print "</tr>"

print "</table>"
