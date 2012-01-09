#!/bin/bash

TARGETS=$(cat TARGETS)
PREFIX=$(cat PREFIX)
mkdir -p $PREFIX
echo ""
echo "Installing Ray to $PREFIX"
echo ""
cp $TARGETS $PREFIX
cp -r Documentation $PREFIX
cp README.md $PREFIX
cp MANUAL_PAGE.txt $PREFIX
cp INSTALL.txt $PREFIX
cp AUTHORS $PREFIX
cp LICENSE.txt $PREFIX

ls $PREFIX
