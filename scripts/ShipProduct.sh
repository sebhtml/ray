#!/bin/bash

release=$1

RayGit=git://github.com/sebhtml/ray.git
RayPlatformGit=git://github.com/sebhtml/RayPlatform.git
ExampleGit=git://github.com/sebhtml/RayPlatform-example.git

mkdir -p ~/ProductReleases/$release
cd ~/ProductReleases/$release

git clone $RayGit Ray-$release
cd Ray-$release
git log|grep ^commit|head -n1|awk '{print $2}' > tag.txt
echo https://github.com/sebhtml/ray > github.txt
echo git://github.com/sebhtml/ray.git > git.txt
rm -rf .git
rm -rf RayPlatform

git clone $RayPlatformGit
cd RayPlatform
git log|grep ^commit|head -n1|awk '{print $2}' > tag.txt
echo https://github.com/sebhtml/RayPlatform > github.txt
echo git://github.com/sebhtml/RayPlatform.git > git.txt
rm -rf .git
cd ..

git clone $ExampleGit
cd RayPlatform-example
git log|grep ^commit|head -n1|awk '{print $2}' > tag.txt
echo https://github.com/sebhtml/RayPlatform-example > github.txt
echo git://github.com/sebhtml/RayPlatform-example.git > git.txt
rm -rf .git
cd ..



cd ..
tar cjf Ray-$release.tar.bz2 Ray-$release
tar czf Ray-$release.tar.gz Ray-$release
zip -yr Ray-$release.zip Ray-$release


