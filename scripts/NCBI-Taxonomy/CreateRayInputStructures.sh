#!/bin/bash
# Author: Sébastien Boisvert
# Date: 2012-09-19

program=$0
OutputDirectory=$1
waitingSeconds=10

if test "$OutputDirectory" = ""
then
	echo "Warning: no output directory was provided, will use NCBI-taxonomy."
	OutputDirectory=NCBI-taxonomy
fi

# From: http://stackoverflow.com/questions/592620/check-if-a-program-exists-from-a-bash-script
command -v wget >/dev/null 2>&1 || { echo >&2 "Error, needs wget but it's not installed.  Aborting."; exit 1; }

echo "Welcome to Ray technolologies !"
echo "How are you today ?"
echo "Good!"
echo ""
echo "This is the easy-to-use assistant for generating NCBI taxonomy files to use with Ray Communities"


echo "Bioinformatics operations will begin in $waitingSeconds seconds..."

sleep $waitingSeconds

echo "OutputDirectory is $OutputDirectory"



echo "We have wget !"



if test -d $OutputDirectory
then
	echo "Warning: directory $OutputDirectory already exists."
else
	mkdir $OutputDirectory
fi

cd $OutputDirectory

if test ! -d ftp.ncbi.nih.gov
then
	mkdir ftp.ncbi.nih.gov
	cd ftp.ncbi.nih.gov

	if test ! -f gi_taxid_nucl.dmp.gz
	then
		echo "Downloading gi_taxid_nucl.dmp.gz, please wait."
		wget ftp://ftp.ncbi.nih.gov/pub/taxonomy/gi_taxid_nucl.dmp.gz
		echo "Done."
	fi

	if test ! -f taxdump.tar.gz
	then
		echo "Downloading taxdump.tar.gz, please wait."
		wget ftp://ftp.ncbi.nih.gov/pub/taxonomy/taxdump.tar.gz
		echo "Done."
	fi

	if test ! -f all.fna.tar.gz
	then
		echo "Downloading all.fna.tar.gz, please wait."
		wget ftp://ftp.ncbi.nih.gov/genomes/Bacteria/all.fna.tar.gz
		echo "Done."
	fi

	cd ..
else
	echo "$OutputDirectory/ftp.ncbi.nih.gov already exists, skipping."
fi

if test ! -d uncompressed
then
	mkdir uncompressed
	cd uncompressed

	echo "Decompressing taxdump.tar.gz, please wait."
	mkdir taxdump
	cd taxdump
	cat ../../ftp.ncbi.nih.gov/taxdump.tar.gz|gunzip|tar -x
	cd ..
	echo "Done."

	echo "Decompressing all.fna.gz, please wait."
	mkdir all.fna
	cd all.fna
	cat ../../ftp.ncbi.nih.gov/all.fna.tar.gz|gunzip|tar -x
	cd ..
	echo "Done."

	cd ..
fi


if test ! -f Genome-to-Taxon.tsv
then
	echo "Creating $OutputDirectory/Genome-to-Taxon.tsv, please wait."
	cat ftp.ncbi.nih.gov/gi_taxid_nucl.dmp.gz|gunzip > Genome-to-Taxon.tsv
	echo "Done."
fi

if test ! -f TreeOfLife-Edges.tsv
then
	echo "Creating $OutputDirectory/TreeOfLife-Edges.tsv, please wait."
	cat uncompressed/taxdump/nodes.dmp|awk '{print $3"\t"$1}' > TreeOfLife-Edges.tsv
	echo "Done."
fi


if test ! -f Taxon-Names.tsv
then
	echo "Creating $OutputDirectory/Taxon-Names.tsv, please wait."
	Create-Taxon-Names.py uncompressed/taxdump/nodes.dmp uncompressed/taxdump/names.dmp Taxon-Names.tsv
	echo "Done."
fi

if test ! -d NCBI-Finished-Bacterial-Genomes
then
	echo "Creating $OutputDirectory/NCBI-Finished-Bacterial-Genomes, please wait."

	mkdir NCBI-Finished-Bacterial-Genomes
	cd NCBI-Finished-Bacterial-Genomes

	for i in $(ls ../uncompressed/all.fna)
	do
		name=$(echo $i|sed 's/_uid/ /g'|awk '{print $1}')
	
		cat ../uncompressed/all.fna/$i/*.fna > $name".fasta"
	done

	echo "Done."
	
	cd ..
fi

echo ""
echo "Finished, checking files... (this should take $waitingSeconds seconds)"

sleep $waitingSeconds

echo ""
echo "We thank you for your patience !"
echo ""
echo "NCBI taxonomy files are ready to be used with Ray Communities !"
echo "If you need more information, you can read these documents from the ray distribution:"
echo "- Documentation/NCBI-Taxonomy.txt (information about the NCBI taxonomy and Ray Communities)"
echo "- Documentation/Taxonomy.txt (information about the general taxonomy features in Ray Communities)"
echo "- Documentation/BiologicalAbundances.txt (information about biological profiling in Ray Communities)"
echo "- MANUAL_PAGE.txt (full list of options of Ray)"

echo "If you need support, send a email to denovoassembler-users@lists.sf.net"

echo ""
echo "Thank you for choosing Ray for your research."
echo "Happy open assembly and profiling to you !"


