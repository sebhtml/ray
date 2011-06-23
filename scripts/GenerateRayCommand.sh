echo "#Generated with $0 on $(date)"
echo "mpirun -np 30 /path/to/Ray  \\"

for i in $(ls *.fast*|grep _1.fastq)
do
	echo "-p \\"
	echo "   $(pwd)/$i \\"
	echo "   $(pwd)/$(echo $i|sed 's/_1\.fast/_2.fast/g') \\"
done


# BGI naming scheme in assemblathon 2
for i in $(ls *.fast*|grep _1.fq.fastq)
do
	echo "-p \\"
	echo "   $(pwd)/$i \\"
	echo "   $(pwd)/$(echo $i|sed 's/_1\.fq.fast/_2.fq.fast/g') \\"
done



# assemblathon format
for i in $(ls *.fast*|grep "\.1\.fastq")
do
	echo "-p \\"
	echo "   $(pwd)/$i \\"
	echo "   $(pwd)/$(echo $i|sed 's/\.1\.fast/.2.fast/g') \\"
done


output=$((ls *.fast*;date)|md5sum|awk '{print $1}')
echo "-o $output | tee $output"".log"
