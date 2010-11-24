for i in $(ls *.sh)
do
	bash $i
done |tee Regressions-$(date +%Y%m%d%H%M%S%s).txt
