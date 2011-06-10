#!/bin/bash

TARGETS=$(cat TARGETS)
PREFIX=$(cat PREFIX)
mkdir -p $PREFIX
echo ""
echo "Installing Ray to $PREFIX"
echo ""
cp $TARGETS $PREFIX
cp InstructionManual.* $PREFIX
cp README.md $PREFIX/README.doc
cp LICENSE $PREFIX/LICENSE.doc
cp ChangeLog $PREFIX/ChangeLog.doc
ls $PREFIX 
