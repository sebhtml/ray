# build Ray executables for system tests

SLOTS=4

module load compilers/gcc/4.4.2 mpi/openmpi/1.4.3_gcc

cd $RAY_GIT_PATH/system-tests

rm -rf builds
mkdir builds

cd $RAY_GIT_PATH

for k in 32 64 96 128
do
	make clean
	echo "$(date) Building k=$k"
	make MAXKMERLENGTH=$k -j $SLOTS PREFIX=$RAY_GIT_PATH/system-tests/builds/build-$k ASSERT=y 
	make install
done

make clean
echo "$(date) Building Ray with compression support"
make -j $SLOTS PREFIX=$RAY_GIT_PATH/system-tests/builds/build-compression HAVE_LIBZ=y HAVE_LIBBZ2=y ASSERT=y 
make install

echo "$(date) Building vanilla Ray"
make clean
make MAXKMERLENGTH=32 ASSERT=y -j $SLOTS

cd $RAY_GIT_PATH/system-tests

