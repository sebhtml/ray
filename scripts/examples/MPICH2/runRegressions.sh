j=1


for i in $(ls *.sh)
do
	echo "RegressionTest Number $j"
	j=$(($j+1))
	echo ""
	echo "FILE = $i"
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
	echo ""
done |tee Regressions-$(date +%Y%m%d%H%M%S%s).txt
