#!/bin/bash

TARGETS=$(cat TARGETS)
PREFIX=$(cat PREFIX)
mkdir -p $PREFIX
echo ""
echo "Installing Ray to $PREFIX"
echo ""
cp $TARGETS $PREFIX
cp -r Documentation $PREFIX

ls $PREFIX
