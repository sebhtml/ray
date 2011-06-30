devTree=~/git-clones/ray

# origin should be ~/RayIntegrationTests
origin=$(pwd)

mkdir builds
rm -rf builds/*

cd $devTree

for k in 32 64 96 128
do
	make clean
	make MAXKMERLENGTH=$k -j 30 PREFIX=$origin/builds/build-$k ASSERT=y
	rm -rf $origin/builds/build-$k
	make install
done

make clean
make -j 30 PREFIX=$origin/builds/build-compression HAVE_LIBZ=y HAVE_LIBBZ2=y ASSERT=y
rm -rf $origin/builds/build-compression
make install

make clean
make MAXKMERLENGTH=32 ASSERT=y -j 30
cd $origin

