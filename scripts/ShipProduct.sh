#!/bin/bash
# product release script
# authored by Sébastien Boisvert

echo "Welcome to Ray Technologies."
echo "Today we ship a product."
echo ""

# you need 

# - git
# - mkdir (coreutils)
# - bash
# - xz
# - tar
# - gzip
# - bzip2
# - zip
# - md5sum
# - sha1sum

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

echo "Cloning $RayGit"
git clone $RayGit $productName &> /dev/null
cd $productName
git log|grep ^commit|head -n1|awk '{print $2}' > tag.txt
echo https://github.com/sebhtml/ray > github.txt
echo git://github.com/sebhtml/ray.git > git.txt
rm -rf .git
rm -rf RayPlatform

echo "Cloning $RayPlatformGit"
git clone $RayPlatformGit &> /dev/null
cd RayPlatform
git log|grep ^commit|head -n1|awk '{print $2}' > tag.txt
echo https://github.com/sebhtml/RayPlatform > github.txt
echo git://github.com/sebhtml/RayPlatform.git > git.txt
rm -rf .git
cd ..



# create archives

echo "Creating product releases."

cd ..
echo ".tar.bz2"
tar cjf $productName.tar.bz2 $productName &> /dev/null

rm -rf $productName

# compute checksums

for i in $(ls Ray-$release.*)
do
	sha1sum $i > $i.sha1sum.txt
done

echo "The product was shipped to $(pwd)"
