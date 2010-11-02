source ../0sim4-parameters.sh
source ../0parameters.sh
newAssembly newbler-run 
cd newbler-run
addRun $reads
runProject 
cd ..
ln -s newbler-run/assembly/454AllContigs.fna Assembly.fasta
echo Newbler>Assembler.txt
ln -s $ref Reference.fasta
