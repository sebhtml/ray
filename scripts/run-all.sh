
for j in abyss ray velvet newbler euler-sr
do
	for i in $(ls 0*$j.sh)
	do
		./run-script.sh $i
	done
done
