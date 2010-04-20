source ../0parameters.sh
source ../0mix13-parameters.sh
newAssembly run 
cd run
addRun $r4541 
addRun $r4542 
addRun $r4543 
runProject 
cd ..
ln -s $ref Reference.fasta
ln -s run/assembly/454AllContigs.fna Assembly.fasta
echo Newbler>Assembler.txt
