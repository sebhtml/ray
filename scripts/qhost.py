#!/usr/bin/python

import sys

if len(sys.argv)!=2:
	print "Provide a SGE Job-ID."
	sys.exit()

jobId=sys.argv[1]
import os
from xml.dom.minidom import parse, parseString

os.system("qhost -j -xml>dump.xml")

dom=parse("dump.xml")

for host in dom.getElementsByTagName("host"):
	hostValues={}
	for hostvalue in host.getElementsByTagName("hostvalue"):
		name=hostvalue.getAttribute("name")
		value=hostvalue.childNodes[0].nodeValue
		hostValues[name]=value
	for job in host.getElementsByTagName("job"):
		jobValues={}
		for jobvalue in job.getElementsByTagName("jobvalue"):
			name=jobvalue.getAttribute("name")
			value=jobvalue.childNodes[0].nodeValue
			jobValues[name]=value
		owner=jobValues["job_owner"]
		if job.getAttribute("name")==jobId:
			print jobValues["queue_name"]+"\t"+jobValues["pe_master"]+"\t"+jobValues["job_name"]+"\t"+job.getAttribute("name")+"\t"+host.getAttribute("name")+"\t"+hostValues["load_avg"]+"\t"+hostValues["num_proc"]+"\t"+hostValues["mem_used"]+"\t"+hostValues["mem_total"]

