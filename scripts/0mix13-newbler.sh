source ../0parameters.sh
source ../0mix13-parameters.sh
newAssembly run &> log1
cd run
addRun $r4541 &> log2
addRun $r4542 &> log3
addRun $r4543 &> log3
runProject &> log4
print-latex.sh $ref assembly/454AllContigs.fna Newbler
