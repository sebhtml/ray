#!/bin/bash

echo "{{{"
echo "#!html"
echo "<table border=\"1\">"
for i in $(cat sets.txt|grep 'o '|awk '{print $2}')
do
	dataset=$(grep $i sets.txt)
	echo "<tr><td colspan=\"11\">$dataset</td></tr>"
	echo "<tr><td>Assembler</td><td>Contigs</td><td>Bases</td><td>Average length</td><td>N50</td><td>Largest length</td><td>Genome breadth of coverage</td><td>Misassembled contigs</td><td>Misassemblies</td><td>Indels</td><td>Running time</td></tr>"
	for j in newbler euler-sr velvet ray abyss
	do
		file=0$i-$j.sh.res.html
		if test -f $file
		then
			cat $file|grep -v applicable|grep -v numberOf
		else
			file=0$i-$j.sh.res
			
			if test -f 0$i-$j.sh.res
			then
				line=$(cat $file|grep -v applicable|grep -v numberOf|sed 's/^/<tr><td>/g')
				exp2='s/&/<\/td><td>/g'
				line=$(echo $line|sed $exp2|sed 's/\\\\/<\/td><\/tr>/g')
				echo $line
			else
				echo "<tr><td>MISSING RESULTS ($j)</td></tr>"
			fi
		fi
	done
done 

echo "</table>"
echo "}}}"
