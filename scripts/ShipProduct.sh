#!/bin/bash
# product release script
# authored by Sébastien Boisvert

echo "Welcome to Ray Technologies."
echo "Today we ship a product."
echo ""

release=$1
base=~/git-clones

RayGit=$base/ray
RayPlatformGit=$base/RayPlatform
productName=Ray-$release
productDirectory=~/ProductReleases/$productName

if test -d $productDirectory
then
	echo "Product $productDirectory is already released !"
	exit 1
fi

echo "Assembling product parts"

mkdir -p $productDirectory
cd $productDirectory

git clone $RayGit $productName &> /dev/null
cd $productName
rm -rf .git
rm -rf RayPlatform

git clone $RayPlatformGit &> /dev/null
cd RayPlatform
rm -rf .git
cd ..
cd ..

echo "Creating product releases."
tar -cjf $productName.tar.bz2 $productName &> /dev/null

rm -rf $productName

for i in $(ls Ray-$release.*)
do
	sha1sum $i > $i.sha1sum.txt
done

echo "The product was shipped to $(pwd)"
