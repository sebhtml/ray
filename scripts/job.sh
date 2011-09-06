##!/bin/bash
#$ -N blat-20110823
#$ -P nne-790-ab
#$ -l h_rt=48:00:00
#$ -pe default 8
#$ -cwd

file=k31-Ray-Bird-20110905-1.Contigs.fasta

ln -s ../$file 

./blat -fastMap -fastMap $file $file selfMap.psl

