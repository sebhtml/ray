
suffix=$(date +%Y-%m-%d-%H-%M-%S-%s).txt

(
echo "BEGIN LIST OF REGRESSIONS"
for i in $(ls *.sh|grep -v runRegressions)
do
	echo "RegressionTest Number $j -> $i"
done
echo "END^^ LIST OF REGRESSIONS"
echo ""
echo ""


for i in $(ls *.sh|grep -v runRegressions)
do
	bash runRegression.sh $i $suffix
done 

) |tee Regressions-$suffix
