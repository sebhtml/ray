#!/bin/bash

TARGETS=$(cat TARGETS)
PREFIX=$(cat PREFIX)
mkdir -p $PREFIX
echo ""
echo "Installing Ray to $PREFIX"
echo ""
cp $TARGETS $PREFIX
if test -f InstructionManual.pdf
then
	cp InstructionManual.pdf $PREFIX
fi
cp README.md $PREFIX/README.doc
cp LICENSE $PREFIX/LICENSE.doc
cp ChangeLog $PREFIX/ChangeLog.doc
ls $PREFIX 
