#!/bin/bash
# test framework
# Sébastien Boisvert


function getHandle(){
	head /dev/urandom|sha1sum|awk '{print $1}'
}

function printFancy(){

	passCount=$1
	failCount=$2
	total=$3
	errors=$4

	echo ""
	echo "PASS	$passCount	/	$total"
	echo "FAIL	$failCount	/	$total"

	echo ""
	echo "error	$errors"
	echo ""

}


file=main-$(date +%Y-%m-%d-%H-%M-%S)-$(getHandle).txt

(

echo "" > UnitTestResults.txt

rm -rf a.out *.log

suites=$(ls *.sh|grep -v main.sh)
suiteCount=$(ls *.sh|grep -v main.sh|wc -l)

date
echo ""
echo "Hello and welcome to Ray Technologies !"
echo "Test Suite Runner here, over and out."
echo ""
echo "Today, we test Ray and RayPlatform."
echo ""
echo "Test suites: $suiteCount"
j=1

for i in $suites
do
	echo ""
	echo "Test Suite Number: $j / $suiteCount"
	j=$(($j+1))

	echo "Test Suite Name: $i"
	echo "Test Suite Instance Name: $(getHandle)"
	echo "Started test suite..."
	rm -rf a.out

	logFile=$(echo $i|sed 's/\.sh/.log/g')

	bash $i &> $logFile
	passCount=$(grep PASS $logFile|wc -l)
	failCount=$(grep FAIL $logFile|wc -l)
	total=$(($passCount+$failCount))
	errors=$(grep error $logFile|wc -l)

	printFancy $passCount $failCount $total $errors 

	echo "Completed test suite..."
	echo "See $logFile"
	echo ""
	echo "-"
done 


passCount=$(grep PASS *.log|wc -l)
failCount=$(grep FAIL *.log|wc -l)
total=$(($passCount+$failCount))
errors=$(grep error *.log|wc -l)

echo "Total."
printFancy $passCount $failCount $total $errors


echo "Thank you, have a nice day."
echo "We hope you visit us again !"

) | tee $file

echo "See $file"


