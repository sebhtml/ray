#!/bin/bash
#
# run system tests in parallel on the grid.
# each system test also runs in parallel.
#
# "I heard you like parallel computing so 
#  I put a parallel computation in a parallel computation so you
#  can parallel compute while you parallel compute."
#       -
# read the README to know more.


export RAY_GIT_PATH=~/git-clones/ray
export PATH=$RAY_GIT_PATH/scripts:$PATH
export PATH=~/software/MUMmer3.22:$PATH
export PATH=~/software/amos-2.0.8/build/bin:$PATH

suffix=$(date +%Y-%m-%d-%H-%M-%S)

echo "$(date) Making fresh Ray builds"
bash makeBuilds.sh

echo -n "" > jobs

echo "$(date) Submitting jobs to the grid"

# prepare a qsub file for each test
# submit the qsub file for each test
# collect the job id and write it to a file
for testName in $(ls tests)
do
	echo "$(date) Preparing submission script for job $testName"
	echo "$(date) Submitting job $testName to the compute grid"
	qsubOut=$testName.qsub
	./run-test-sge.sh $testName &> $qsubOut
	jobId=$(cat $qsubOut|awk '{print $3}')
	echo $jobId >> jobs
	echo "$(date) Job $testName has id $jobId"
done 

echo "$(date) Done submitting jobs."

# blocks until all jobs in job are completed

cp jobs jobs-not-finished

# check that all jobs listed in the file are completed (not running and not queued
while test $(wc -l jobs-not-finished|awk '{print $1}') -ne 0
do
	echo "$(date) Remaining jobs: $(wc -l jobs-not-finished|awk '{print $1}')"
	echo -n "" > jobs-not-finished

	for job in $(cat jobs)
	do
		qstat|grep $job >> jobs-not-finished
	done
	sleep 1
done

echo "$(date) All jobs have completed."

outputFile=SystemTests-$suffix

# TODO: remove this part because ruby is now available on the grid
# ruby is not available on the grid nodes
# therefore, ValidateGenomeAssembly must be re-run.

echo "$(date) Validating genomes (not necessary if ruby is available on the grid nodes)."

echo -n "" > $outputFile
echo -n "" > $outputFile.validationEntries

for testName in $(ls tests)
do
	echo "$(date) Validating job $testName"
	cd tests/$testName
	cat main.sh|grep ValidateGenome > validate.sh
	TEST_NAME=$testName
	source validate.sh > validate.out
	cat validate.out >> ../../$outputFile.validationEntries
	echo "" >> ../../$outputFile.validationEntries
	echo "$(date) Job $testName has been validated."
	cd ../..
done &> ValidationErrors.txt

for job in $(cat jobs)
do
	cat ~/$(ls ~|grep $job|grep "\.o") >> $outputFile
done

summarize-regressions.py $outputFile.validationEntries > $outputFile.html

echo "$(date) summary of system tests: $outputFile.html"
echo "$(date) logs are in $outputFile"
echo "$(date) validation entries: $outputFile.validationEntries"
