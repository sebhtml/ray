echo "#Generated with $0 on $(date)"
echo "mpirun -np 30 /path/to/Ray  \\"

for i in $(ls *.fast*|grep _1)
do
	echo "-p \\"
	echo "   $(pwd)/$i \\"
	echo "   $(pwd)/$(echo $i|sed 's/_1\.fast/_2.fast/g') \\"
done

output=$((ls *.fast*;date)|md5sum|awk '{print $1}')
echo "-o $output"
