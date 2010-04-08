
for i in $(ls 0*ray.sh)
do
	echo $i
	./run-script.sh $i
done

