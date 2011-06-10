origin=$(pwd)
cd ../..
for k in 32 64 96 128
do
	make clean
	make MAXKMERLENGTH=$k -j 30 PREFIX=$origin/build-$k
	rm -rf $origin/build-$k
	make install
done
make clean
make -j 30
cd $origin

