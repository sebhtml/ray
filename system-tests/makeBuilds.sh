# build Ray executables for system tests

cd $RAY_GIT_PATH/system-tests

rm -rf builds
mkdir builds

cd $RAY_GIT_PATH

for k in 32 64 96 128
do
	make clean
	echo "$(date) Building k=$k"
	make MAXKMERLENGTH=$k -j 30 PREFIX=$RAY_GIT_PATH/system-tests/builds/build-$k ASSERT=y &> /dev/null
	make install
done

make clean
echo "$(date) Building Ray with compression support"
make -j 30 PREFIX=$RAY_GIT_PATH/system-tests/builds/build-compression HAVE_LIBZ=y HAVE_LIBBZ2=y ASSERT=y &> /dev/null
make install

echo "$(date) Building vanilla Ray"
make clean
make MAXKMERLENGTH=32 ASSERT=y -j 30 &> /dev/null

cd $RAY_GIT_PATH/system-tests

