#!/usr/bin/python

import sys


lastHost=""
color1="FFFFCC"
color2="CCFFFF"
currentColor=color1

# mpi@r101-n81    SLAVE   Illumina        1072283 r101-n81        7.98    8       2.5G    23.5G

print "<html><head><title>Utilization of Resources</title><style>td,th{border-width: 1px; border-style: solid;}</style></head><body>"

print "<br />"
print "<br />"
print "<br />"
print "This file was generated with qhost.py and add-color-to-qhost.py distributed with Ray, http://denovoassembler.sf.net/\n\n\n\n"
print "<br />"
print "<br />"

print "<center>"

print "<table width=\"800\"><caption>Utilization of Resources</caption><tbody>"

print "<tr><th>Global identifier</th><th>Per-host identifier</th><th>Host-specific queue-name</th><th>Type</th><th>Job name</th><th>Job identifier</th><th>Host name</th><th>Average load</th><th>Number of processor cores</th><th>Host-specific utilized memory</th><th>Host-specific available memory</th></tr>"

count=0
globalCount=1
lastType=""

for line in sys.stdin:
	tokens=line.split()
	if tokens[0]=="":
		continue

	if lastHost=="":
		lastHost=tokens[0]

	if tokens[0]!=lastHost and lastType!="MASTER":
		count=1
		if currentColor==color1:
			currentColor=color2
		else:
			currentColor=color1
	elif lastType!="MASTER":
		count+=1

	loadToken=tokens[5]
	if loadToken=="-":
		loadToken="-1"
	load=float(loadToken)
	processors=float(tokens[6])
	color="green"

	full=""

	if load>processors*1.03:
		color="red"
	print "<tr style=\"background-color: "+currentColor+"\"><td>"+str(globalCount)+"</td><td>"+str(count)+full+"</td><td><u>"+tokens[0]+"</u></td><td>"+tokens[1]+"</td><td><i>"+tokens[2]+"</i></td><td>"+tokens[3]+"</td><td>"+tokens[4]+"</td><td><b><span style=\"color: "+color+";\">"+tokens[5]+"</span></b></td><td>"+tokens[6]+"</td><td>"+tokens[7]+"</td><td>"+tokens[8]+"</td></tr>"

	lastHost=tokens[0]
	if tokens[1]!="MASTER":
		globalCount+=1
	lastType=tokens[1]

print "</tbody></table>"

print "</center></body></html>"
