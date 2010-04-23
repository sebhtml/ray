#!/bin/bash

i=$1
print-latex.sh $i.dir/Reference.fasta $i.dir/Assembly.fasta $(cat $i.dir/Assembler.txt)|grep -v '%'> 1
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
wallclock=$(cat $i.dir/WallClockTime|grep real|awk '{print $2}')

echo "<tr><td>$assembler</td><td>$contigs</td><td>$bases</td><td>$mean</td><td>$n50</td><td>$max</td><td>$coverage</td><td>$misass</td><td>$mismat</td><td>$indels</td><td>$wallclock</td></tr>" > $i.res.html
echo "$assembler & $contigs & $bases & $mean & $n50 & $max & $coverage & $misass & $mismat & $indels & $wallclock \\\\" > $i.res
echo "Wrote $i.res and $i.res.html"
