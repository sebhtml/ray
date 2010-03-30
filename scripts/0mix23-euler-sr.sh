source ../0parameters.sh
source ../0mix2-parameters.sh


cat $file1illumina $file2illumina > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta 1 -type sanger &>log1
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4541 > 2
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4542 > 3
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4543 > 4
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4544 > 5
cat 1 2 3 4 5 > reads.fasta 
Assemble.pl reads.fasta $wordSize &> log1
print-latex.sh $ref reads.fasta.contig EULER-SR

