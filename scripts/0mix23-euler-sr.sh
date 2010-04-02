source ../0parameters.sh
source ../0mix2-parameters.sh
cat $file1illumina $file2illumina > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta r1 -type sanger &>log1
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4541 > r2
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4542 > r3
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4543 > r4
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4544 > r5
cat r1 r2 r3 r4 r5 > reads.fasta 
Assemble.pl reads.fasta $wordSize &> log1
print-latex.sh $ref reads.fasta.contig EULER-SR

