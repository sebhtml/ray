g++ coverage_main.cpp ../code/graph/CoverageDistribution.cpp -O3 -o CoverageExe -I ../code -I ..

for i in $(ls CoverageDistribution/*.expected)
do
	./CoverageExe $(echo $i|sed 's/.expected//g') $i
done> cov.log

echo "Passed $(grep Passed cov.log|grep Test|wc -l) tests"
echo "Failed $(grep Failed cov.log|grep Test|wc -l) tests"
grep Failed cov.log|grep Test
echo "see cov.log"
