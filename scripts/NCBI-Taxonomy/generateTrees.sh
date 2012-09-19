#!/bin/bash

PATH=.:$PATH

# viruses
mkdir -p Viruses-Genomes
rm Viruses-Genomes/* -f

mkdir -p Viruses-ProteinCodingGenes
rm Viruses-ProteinCodingGenes/* -f

mkdir -p Viruses-RNAGenes
rm Viruses-RNAGenes/* -f

for i in $(ls Viruses|sort -r -n)
do
	#genomes
	for j in $(ls Viruses/$i/*|grep .fna$|sort -r -n)
	do
		name=$(getName.py $j)

		if test $(ls Viruses/$i/*.fna|wc -l) -gt 1
		then
			name=$(getNameInFile.py $j)
		fi

		if test -f Viruses-Genomes/$name.fasta
		then
			name=$name"_"$(basename j)
		fi

		ln -s ../$j Viruses-Genomes/$name.fasta
	done

	# protein-coding genes
	for j in $(ls Viruses/$i/*|grep .ffn$|sort -r -n)
	do
		name=$(getName.py $j)

		if test $(ls Viruses/$i/*.ffn|wc -l) -gt 1
		then
			name=$(getNameInFile.py $j)
		fi

		if test -f Viruses-ProteinCodingGenes/$name.fasta
		then
			name=$name"_"$(basename j)
		fi

		ln -s ../$j Viruses-ProteinCodingGenes/$name.fasta
	done

	# RNA genes
	for j in $(ls Viruses/$i/*|grep .frn$|sort -r -n)
	do
		name=$(getName.py $j)

		if test $(ls Viruses/$i/*.frn|wc -l) -gt 1
		then
			name=$(getNameInFile.py $j)
		fi

		if test -f Viruses-RNAGenes/$name.fasta
		then
			name=$name"_"$(basename j)
		fi

		ln -s ../$j Viruses-RNAGenes/$name.fasta
	done
done



# bacteria
mkdir -p Bacteria-Genomes
rm Bacteria-Genomes/* -f

mkdir -p Bacteria-ProteinCodingGenes
rm Bacteria-ProteinCodingGenes/* -f

mkdir -p Bacteria-RNAGenes
rm Bacteria-RNAGenes/* -f

for i in $(ls Bacteria|sort -r -n)
do
	#genomes
	for j in $(ls Bacteria/$i/*.fna|sort -r -n)
	do
		name=$(getName.py $j)

		if test $(ls Bacteria/$i/*.fna|wc -l) -gt 1
		then
			name=$(getNameInFile.py $j)
		fi

		if test -f  Bacteria-Genomes/$name.fasta
		then
			name=$name"_"$(basename j)
		fi

		ln -s ../$j Bacteria-Genomes/$name.fasta
	done

	# protein-coding genes
	for j in $(ls Bacteria/$i/*.ffn|sort -r -n)
	do
		name=$(getName.py $j)

		if test $(ls Bacteria/$i/*.ffn|wc -l) -gt 1
		then
			genomeFile=$(echo $j|sed 's/.ffn/.fna/g')
			name=$(getNameInFile.py $genomeFile)
		fi

		if test -f  Bacteria-ProteinCodingGenes/$name.fasta
		then
			name=$name"_"$(basename j)
		fi

		ln -s ../$j Bacteria-ProteinCodingGenes/$name.fasta
	done

	# RNA genes
	for j in $(ls Bacteria/$i/*.frn|sort -r -n)
	do
		name=$(getName.py $j)

		if test $(ls Bacteria/$i/*.frn|wc -l) -gt 1
		then
			genomeFile=$(echo $j|sed 's/.frn/.fna/g')
			name=$(getNameInFile.py $genomeFile)
		fi

		if test -f  Bacteria-RNAGenes/$name.fasta
		then
			name=$name"_"$(basename j)
		fi

		ln -s ../$j Bacteria-RNAGenes/$name.fasta
	done
done


