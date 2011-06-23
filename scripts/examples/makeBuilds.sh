origin=$(pwd)
cd ../..
for k in 32 64 96 128
do
	make clean
	make MAXKMERLENGTH=$k -j 30 PREFIX=$origin/build-$k ASSERT=y
	rm -rf $origin/build-$k
	make install
done

make clean
make -j 30 PREFIX=$origin/build-compression HAVE_LIBZ=y HAVE_LIBBZ2=y ASSERT=y
rm -rf $origin/build-compression
make install

make clean
make MAXKMERLENGTH=32 ASSERT=y -j 30
cd $origin

