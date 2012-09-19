#!/usr/bin/env python
#encoding: UTF-8
# author: Sébastien Boisvert

"""
GenerateTaxonNames.py Taxon-Names.tsv Taxon-Types.tsv > NCBI-taxons-for-Ray.txt


 Genome-to-Taxon.tsv -> ../2012-01-23/gi_taxid_nucl.dmp
 Taxon-Names.tsv -> ../2011-11-05/ncbi.map
 Taxon-Types.tsv -> ../2011-11-05/ncbi.lvl
 TreeOfLife-Edges.tsv -> ../2011-11-05/TreeOfLife-Edges.tsv


"""

import sys

if len(sys.argv)!=3:
	print __doc__
	sys.exit(1)

namesFile=sys.argv[1]
typesFile=sys.argv[2]

types={}

for line in open(typesFile):
	tokens=line.split("\t")
	type=int(tokens[0])

	name=tokens[1].strip()

	types[type]=name

for line in open(namesFile):
	tokens=line.split("\t")
	type=int(tokens[3])
	
	taxonNumber=tokens[0]
	taxonName=tokens[1]

	rank="CachingError"

	if type in types:
		rank=types[type]

	print taxonNumber+"	"+taxonName+"	"+rank
