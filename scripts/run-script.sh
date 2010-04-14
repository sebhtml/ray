#!/bin/bash

i=$1
prefix=$(pwd)
echo "Running $i"
if test -f $i.res
then
	echo "$i is already done. ($i.res)"
elif test -d $i.dir
then
	echo "$i is already running. ($i.dir)"
else
	echo "Starting $i."
	mkdir $i.dir -p
	cd $i.dir
	(time bash ../$i > Log) &> WallClockTime
	print-latex.sh Reference.fasta Assembly.fasta $(cat Assembler.txt)|grep -v '%'> 1
	assembler=$(cat 1|awk '{print $1}')
	contigs=$(cat 1|awk '{print $3}')
	bases=$(cat 1|awk '{print $5}')
	mean=$(cat 1|awk '{print $7}')
	n50=$(cat 1|awk '{print $9}')
	max=$(cat 1|awk '{print $11}')
	coverage=$(cat 1|awk '{print $13}')
	misass=$(cat 1|awk '{print $15}')
	mismat=$(cat 1|awk '{print $17}')
	indels=$(cat 1|awk '{print $19}')
	wallclock=$(cat WallClockTime|grep real|awk '{print $2}')
	
	echo "<tr><td>$assembler</td><td>$contigs</td><td>$bases</td><td>$mean</td><td>$n50</td><td>$max</td><td>$coverage</td><td>$misass</td><td>$mismat</td><td>$indels</td><td>$wallclock</td></tr>" > ../$i.res.html
	echo "$assembler & $contigs & $bases & $mean & $n50 & $max & $coverage & $misass & $mismat & $indels & $wallclock \\\\" > ../$i.res

	echo "$i is done."
	cd ..
fi

