source ../0parameters.sh
source ../0mix13-parameters.sh
newAssembly run 
cd run
addRun $r4541 
addRun $r4542 
addRun $r4543 
sed -i 's/<overlapSeedHitLimit>10000/<overlapSeedHitLimit>1000/g' assembly/454AssemblyProject.xml
runProject 
cd ..
ln -s $ref Reference.fasta
ln -s run/assembly/454AllContigs.fna Assembly.fasta
echo Newbler>Assembler.txt
