#!/usr/bin/python

#>gi|148243635|ref|NC_009467.1| Acidiphilium cryptum JF-5 plasmid pACRY01, complete sequence

import sys

file=sys.argv[1]

tokens=file.split('/')

tokens2=tokens[1].split("_uid")
name=tokens2[0].strip()

print name
