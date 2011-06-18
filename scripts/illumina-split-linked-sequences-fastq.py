#!/usr/bin/python

import sys

fastqFile=sys.argv[1]

adaptorSequence=sys.argv[2]

out=open(fastqFile+".fasta","w+")

i=0
id1=""
seq1=""
seq2=""

for line in open(fastqFile):
	if i==0:
		id1=line.strip().replace("@",">")
	elif i==1:
		seq1=line.strip()
	elif i==4:
		id2=line.strip().replace("@",">")
	elif i==5:
		seq2=line.strip()
		tokens1=seq1.split(adaptorSequence)
		tokens2=seq2.split(adaptorSequence)
		if len(tokens1)>1:
			seq1=tokens1[0]
		if len(tokens2)>1:
			seq2=tokens2[0]
		out.write(id1+" length="+str(len(seq1))+"\n")
		out.write(seq1+"\n")
		out.write(id2+" length="+str(len(seq2))+"\n")
		out.write(seq2+"\n")


	i+=1
	if i==8:
		i=0

out.close()
