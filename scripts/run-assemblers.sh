
while true
do
	for i in $(ls 0*.sh|grep -v parame)
	do
		bash run-script.sh $i
	done
	sleep 10
done
