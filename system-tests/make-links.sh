for i in $(ls .|grep -v .sh)
do
	cd $i
	for j in $(ls ../../data-for-system-tests/$i/*)
	do
	rm $(basename $j)
	ln -s $j
	done
	cd ..
done
