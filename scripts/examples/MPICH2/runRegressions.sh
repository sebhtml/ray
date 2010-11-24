
log=Regressions-$(date +%Y-%m-%d-%H-%M-%S-%s).txt

(
j=1
echo "BEGIN LIST OF REGRESSIONS"
for i in $(ls *.sh|grep -v runRegressions)
do
	echo "RegressionTest Number $j -> $i"
	j=$(($j+1))
done
echo "END^^ LIST OF REGRESSIONS"
echo ""
echo ""

j=1

for i in $(ls *.sh|grep -v runRegressions)
do
	echo "RegressionTest Number $j -> $i"
	j=$(($j+1))
	echo ""
	echo "FILE = $i"
	date
	echo ""
	echo "OH HAI"
	echo ""
	echo "======== BEGIN >>>"
	echo ""
	bash $i
	echo ""
	echo "======== END^^ >>>"
	echo ""
	echo "K THX BYE"
	echo ""
	echo "..."
	date
	echo ""
done 

) |tee $log
