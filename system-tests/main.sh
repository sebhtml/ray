#!/bin/bash


suffix=$(date +%Y-%m-%d-%H-%M-%S).txt

bash makeBuilds.sh

echo "" > jobs

for testName in $(ls tests)
do
	qsubFile=template.sh-qsub
	cp template.sh $qsubFile
	expression="'s/__TEST_NAME__/$testName/g"
	sed -i $expression $qsubFile
	qsub &> qsub-out
	cat qsub-out|awk '{print $3}' >> jobs
done 

# blocks until all jobs in job are completed

cp jobs jobs-not-finished

while test $(wc -l jobs-not-finished) -ne 0
do
	echo "" > jobs-not-finished

	for job in $(cat jobs)
	do
		qstat|grep $job >> jobs-not-completed
	done
done

outputFile=SystemTests-$suffix

echo "" > 

for job in $(cat jobs)
do
	cat $(ls ~|grep $job|grep "\.o") >> $outputFile
done

summarize-regressions.py $outputFile > $outputFile.html
