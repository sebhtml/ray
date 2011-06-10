origin=$(pwd)
cd ../..
for k in 32 64 96 128
do
	make clean
	make MAXKMERLENGTH=$k -j 30
	make install PREFIX=$origin/build-$k
done
make clean
make -j 30
cd $origin

