source ../0parameters.sh
source ../0mix13-parameters.sh
newAssembly run 
cd run
addRun $r4541 
addRun $r4542 
addRun $r4543 
sed -i 's/<overlapSeedStep>12/<overlapSeedStep>21/g' assembly/454AssemblyProject.xml
sed -i 's/<overlapSeedLength>16/<overlapSeedLength>21/g' assembly/454AssemblyProject.xml
runProject 
cd ..
ln -s $ref Reference.fasta
ln -s run/assembly/454AllContigs.fna Assembly.fasta
echo Newbler>Assembler.txt
