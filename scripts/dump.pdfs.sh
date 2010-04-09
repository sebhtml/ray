for i in $(ls|grep ray|grep dir)
do
	echo $i
	cd $i
	optimize.py Ray-CoverageDistribution.txt
	mv Ray-CoverageDistribution.txt.pdf ../PDFs/$i.pdf
	cd ..
done
