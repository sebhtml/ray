echo "mpirun -np 1 /path/to/Ray -k 19 \\"

for i in $(ls *.fastq|grep _1)
do
	echo "-p \\"
	echo "   $(pwd)/$i \\"
	echo "   $(pwd)/$(echo $i|sed 's/_1/_2/g') \\"
done
