j=1

log=Regressions-$(date +%Y-%m-%d-%H-%M-%S-%s).txt

for i in $(ls *.sh)
do
	echo "RegressionTest Number $j"
	j=$(($j+1))
	echo ""
	echo "FILE = $i"
	echo ""
	echo "OH HAI $(date)"
	echo ""
	echo "======== BEGIN >>>"
	echo ""
	bash $i
	echo ""
	echo "======== END^^ >>>"
	echo ""
	echo "K THX BYE $(date)"
	echo ""
	echo "..."
	echo ""
done |tee $log
