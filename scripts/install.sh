#!/bin/bash

TARGETS=$(cat TARGETS)
PREFIX=$(cat PREFIX)

mkdir -p $PREFIX

echo ""
echo "Installing Ray to $PREFIX"
echo ""

# legal stuff

cp LICENSE.txt $PREFIX
cp gpl-3.0.txt $PREFIX
cp RayPlatform/lgpl-3.0.txt $PREFIX



cp $TARGETS $PREFIX

cp -r Documentation $PREFIX
cp README.md $PREFIX
cp MANUAL_PAGE.txt $PREFIX
cp INSTALL.txt $PREFIX
cp AUTHORS $PREFIX

#cp code/Application/libRayApplication.a $PREFIX
#cp code/Platform/libRayPlatform.a $PREFIX

cp -r scripts/xsl-xml $PREFIX

ls $PREFIX
