source ../0parameters.sh
source ../0mix2-parameters.sh
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4541 > 1.fasta
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4542 > 2.fasta
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4543 > 3.fasta
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4544 > 4.fasta
cat 1.fasta 2.fasta 3.fasta 4.fasta > reads.fasta 
Assemble.pl reads.fasta $wordSize
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR>Assembler.txt

