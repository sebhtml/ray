source ../0parameters.sh
source ../0mix2-parameters.sh

newAssembly run &> log1
cd run
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4541 > 1.fasta
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4542 > 2.fasta
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4543 > 3.fasta
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4544 > 4.fasta

addRun 1.fasta &> log2
addRun 2.fasta &> log3
addRun 3.fasta &> log4
addRun 4.fasta &> log5
runProject &> log6
print-latex.sh $ref assembly/454AllContigs.fna Newbler
