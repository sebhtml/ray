#!/usr/bin/python

import sys

file=sys.argv[1]
output=sys.argv[2]

title=""
text=""
i=0
for line in open(file):
	if i==0:
		title=line
	else:
		text+=line
	i+=1

f=open(sys.argv[2])
content=f.read()

content=content.replace("SCAFFOLD_TITLE",title)
content=content.replace("SCAFFOLD_CONTENT",text)

f=open(output,"w+")
f.write(content)
f.close()


