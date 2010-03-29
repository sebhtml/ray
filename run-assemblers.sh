#!/bin/bash

nproc=28
velvetVersion=0.7.61
eulerVersion=1.1.2
newblerVersion=2.0.00.20
rayVersion=0.0.5
abyssVersion=1.1.2
wordSize=21

adp1=/home/boiseb01/nuccore/adp1.fasta

echo "Mixed set 1: E. coli K-12 MG1655 Illumina paired-end reads (genome: nuccore/NC_000913, reads: sra/SRA001125 + sra/SRA001028) "
testName=SRA001125-Illumina-paired
p1left=/data/users/boiseb01/PaperDatasets/SRA001125/200xSRR001665_1.fastq
p1right=/data/users/boiseb01/PaperDatasets/SRA001125/200xSRR001665_2.fastq
length1=215
sd1=20
p2left=/data/users/boiseb01/PaperDatasets/SRA001125/200xSRR001666_1.fastq
p2right=/data/users/boiseb01/PaperDatasets/SRA001125/200xSRR001666_2.fastq
length2=215
sd2=20
mg1655=/home/boiseb01/nuccore/Ecoli-k12-mg1655.fasta

time abyss-pe k=$wordSize n=10 name=mg1655 lib="lib1 lib2" lib1="$p1left $p1right" lib2="$p2left $p2right"  &>/dev/null
print-latex.sh  $mg1655 mg1655-contigs.fa ABYSS

time(
renameReads-for-euler.py /1 $p1left ;renameReads-for-euler.py /2 $p1right ;renameReads-for-euler.py /1 $p2left ; renameReads-for-euler.py /2 $p2right > reads.fq
qualityTrimmer  -fasta reads.fq -outFasta reads.fasta -type illumina  &>/dev/null
echo '"([^/]*)/([12])" CloneLength=215 CloneVar=20 Type=1      # Illumina mate-pair' > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info &>/dev/null )
print-latex.sh $mg1655 Euler-reads.fasta.contig EULER-SR

time mpirun -np $nproc Ray -p $p1left $p1right $length1 $sd1 -p $p2left $p2right $length2 $sd2
print-latex.sh $mg1655 Ray-Contigs.fasta Ray

time(
rm -rf velvet
shuffleSequences_fastq.pl $p1left $p1right 1
shuffleSequences_fastq.pl $p2left $p2right 2
cat 1 2 > reads.fastq
velveth velvet 21 -shortPaired reads.fastq &>/dev/null
velvetg velvet -ins_length 215 -ins_length_sd 20 -exp_cov 109  -cov_cutoff 10 &>/dev/null
)
print-latex.sh $mg1655 velvet/contigs.fa Velvet

echo "Mixed set 2: Acinetobacter sp. ADP1 (genome: nuccore/NC_005966, reads: sra/SRA003611) -- Illumina reads"
testName=SRA003611-Illumina
file1illumina=/data/users/boiseb01/PaperDatasets/SRA003611/Illumina/SRR006331.fastq
file2illumina=/data/users/boiseb01/PaperDatasets/SRA003611/Illumina/SRR006332.fastq

time ABYSS -k$wordSize $file1illumina $file2illumina -o abyss-contigs &>/dev/null
print-latex.sh $adp1 abyss-contigs abyss

time (cat $file1illumina $file2illumina > reads.fastq
qualityTrimmer  -fasta reads.fastq -outFasta reads.fasta -type illumina &>/dev/null
Assemble.pl reads.fasta $wordSize &>/dev/null) 
print-latex.sh $adp1 Euler-reads.fasta.contig EULER-SR

time mpirun -np $nproc Ray -s $file1illumina -s $file2illumina &>/dev/null
print-latex.sh $adp1 Ray-Contigs.fasta Ray

time (velveth velvet-$testName $wordSize -short -fastq $file1illumina -short -fastq $file2illumina &>/dev/null
velvetg velvet-$testName -exp_cov 51 -cov_cutoff 10 &>/dev/null)
print-latex.sh $adp1 velvet-$testName/contigs.fa Velvet

echo "#  Pseudomonas syringae pathovar syringae B728a (genome: NC_007005.1 reads: era/ERA000095) (400/20) "
left=/data/users/sra/ERA000095/ERR005143_1.fastq 
right=/data/users/sra/ERA000095/ERR005143_2.fastq
length=400
sd=40
syrin=/home/boiseb01/nuccore/Pseudomonas-syringae-pv.-syringae-B728a.fasta

time abyss-pe k=$wordSize n=5 name=syrin lib="lib1" lib1="$left $right" &>/dev/null
print-latex.sh $syrin syrin-contigs.fa ABYSS

time(
renameReads-for-euler.py "" $left ; renameReads-for-euler.py "" $right > reads.fq
qualityTrimmer  -fasta reads.fq -outFasta reads.fasta -type illumina  &>/dev/null
echo '"([^/]*)/([12])" CloneLength=400 CloneVar=40 Type=1      # Illumina mate-pair' > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info &>/dev/null )
print-latex.sh $syrin  Euler-reads.fasta.contig EULER-SR

time mpirun -np $nproc Ray -p $left $right $length $sd &> /dev/null
print-latex.sh $syrin Ray-Contigs.fasta Ray

time(
rm -rf velvet
shuffleSequences_fastq.pl $left $right reads.fastq
velveth velvet 21 -shortPaired reads.fastq &>/dev/null
velvetg velvet -ins_length 400 -cov_cutoff 7 -exp_cov 13  &>/dev/null
)
print-latex.sh $syrin velvet/contigs.fa Velvet
