#!/usr/bin/python

#>gi|148243635|ref|NC_009467.1| Acidiphilium cryptum JF-5 plasmid pACRY01, complete sequence

import sys

file=sys.argv[1]

line=open(file).readline()

tokens=line.split("|")

name=tokens[4].replace(', complete sequence','').replace(', complete chromosome','').strip()
name=name.replace(', complete genome','')
name=name.replace(" ","_").replace('/','_').strip()


print name
