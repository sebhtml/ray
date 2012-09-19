#!/bin/bash
# Author: Sébastien Boisvert
# Date: 2012-09-19

program=$0
OutputDirectory=$1

# From: http://stackoverflow.com/questions/592620/check-if-a-program-exists-from-a-bash-script
command -v wget >/dev/null 2>&1 || { echo >&2 "Error, needs wget but it's not installed.  Aborting."; exit 1; }


if test "$OutputDirectory" = ""
then
	echo "Error: no output directory was provided, please provide a directory."
	echo "Usage: $program OutputDirectoryName"
	exit 1
fi

if test -d $OutputDirectory
then
	echo "Error: the output directory $OutputDirectory already exists, will not overwrite."
	exit 1
fi

mkdir $OutputDirectory
cd $OutputDirectory

if test ! -d ftp.ncbi.nih.gov
then
	mkdir ftp.ncbi.nih.gov
	cd ftp.ncbi.nih.gov

	if test ! -f gi_taxid_nucl.dmp.gz
	then
		wget ftp://ftp.ncbi.nih.gov/pub/taxonomy/gi_taxid_nucl.dmp.gz
	fi

	if test ! -f taxdump.tar.gz
	then
		wget ftp://ftp.ncbi.nih.gov/pub/taxonomy/taxdump.tar.gz
	fi

	if test ! -f all.fna.tar.gz
	then
		wget ftp://ftp.ncbi.nih.gov/genomes/Bacteria/all.fna.tar.gz
	fi

	cd ..
fi

if test ! -d uncompressed
then
	mkdir uncompressed
	cd uncompressed

	mkdir taxonomy
	cd taxonomy
	cat ../../ftp.ncbi.nih.gov/taxdump.tar.gz|gunzip|tar -x
	cd ..

	mkdir 

	cd ..
fi


if test ! -d for-ray
then
	mkdir for-ray
	cd for-ray

	if test ! -d NCBI-Finished-Bacterial-Genomes
	then
		mkdir NCBI-Finished-Bacterial-Genomes
	fi

	if test ! -d NCBI-Draft-Bacterial-Genomes
	then
		mkdir NCBI-Draft-Bacterial-Genomes
	fi

	if test ! -d NCBI-Taxonomy
	then
		mkdir NCBI-Taxonomy
		cd NCBI-Taxonomy

		cat ../../ftp.ncbi.nih.gov/gi_taxid_nucl.dmp.gz|gunzip > Genome-to-Taxon.tsv

		cat ../../ftp.ncbi.nih.gov/nodes.dmp|awk '{print $1"\t"$3}' > TreeOfLife-Edges.tsv

		CreateTaxonNames.py


		cd ..
	fi

	cd ..
fi
