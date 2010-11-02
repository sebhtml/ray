source ../0parameters.sh
source ../0mix32-parameters.sh
newAssembly run 
cd run
addRun $r4541 
addRun $r4542 
addRun $r4543 
addRun $r4544 
addRun $r4545 
runProject
cd ..
ln -s run/assembly/454AllContigs.fna Assembly.fasta
ln -s $ref Reference.fasta
echo Newbler>Assembler.txt
