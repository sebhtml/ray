for i in \
~/CompletedJobs/Ecoli-THEONE.CoverageDistribution.txt \
~/CompletedJobs/strept2.CoverageDistribution.txt \
~/big-genome-2011-05-12.CoverageDistribution.txt \
~/d7e82c74efaff4a591b6b1a379ac8962.CoverageDistribution.txt \
~/SRA000271-HumanGenome-r4483.CoverageDistribution.txt \
~/SRA000271-k=31.CoverageDistribution.txt
do
	echo ""
	echo "==="
	echo $i
	python FindPeak.py $i
done
