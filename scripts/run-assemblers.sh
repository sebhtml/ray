
while true
do
	for i in $(ls 0*.sh|grep -v parame)
	do
		if test -f $i.res
		then
			true
		else
			mkdir $i.dir -p
			cd $i.dir
			echo "Running $i"
			#bash ../$i > ../$i.res
			cd ..
			rm -rf $i.dir
		fi
	done
	sleep 10
done
