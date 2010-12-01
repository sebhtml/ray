rm *.fasta

suffix=$(date +%Y-%m-%d-%H-%M-%S).txt

(
echo "StartingTime= $(date)"

echo "BEGIN LIST OF REGRESSIONS"
for i in $(ls *.sh|bash listRegressions.sh)
do
	echo "RegressionTest Number $j -> $i"
done
echo "END^^ LIST OF REGRESSIONS"
echo ""
echo ""


for i in $(bash listRegressions.sh)
do
	bash runRegression.sh $i $suffix
done 

echo "EndingTime= $(date)"
) | tee Regressions-$suffix
