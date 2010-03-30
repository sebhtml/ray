source ../0sim4-parameters.sh
source ../0parameters.sh

newAssembly newbler-run &> log1
cd newbler-run
addRun $reads &> log2
runProject &> log3
print-latex $ref assembly/454AllContigs.fna Newbler
