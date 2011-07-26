#!/bin/bash

if test $# -eq 0
then
	echo "usage"
	echo "ValidateGenomeAssembly.sh reference assembly assembler"
	exit
fi

assembler=$3
reference=$1
assembly=$2

_ValidateGenomeAssembly.sh $reference $assembly $assembler 500
