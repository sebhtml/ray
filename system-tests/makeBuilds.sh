source config

cd $RAY_GIT_PATH/system-tests

rm -rf builds/*

cd $RAY_GIT_PATH

for k in 32 64 96 128
do
	make clean
	make MAXKMERLENGTH=$k -j 30 PREFIX=$RAY_GIT_PATH/system-tests/builds/build-$k ASSERT=y
	make install
done

make clean
make -j 30 PREFIX=$RAY_GIT_PATH/system-tests/builds/build-compression HAVE_LIBZ=y HAVE_LIBBZ2=y ASSERT=y
make install

make clean
make MAXKMERLENGTH=32 ASSERT=y -j 30

cd $RAY_GIT_PATH/system-tests

