source ../0parameters.sh
source ../0mix2-parameters.sh
newAssembly run &> log1
cd run
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4541 > 1.fasta
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4542 > 2.fasta
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4543 > 3.fasta
awk '/^@SR/{gsub(/^@/,">",$1);print;getline;print}' $r4544 > 4.fasta
addRun 1.fasta
addRun 2.fasta
addRun 3.fasta
addRun 4.fasta
runProject 
cd ..
ln -s $ref Reference.fasta
ln -s run/assembly/454AllContigs.fna Assembly.fasta
echo Newbler>Assembler.txt
