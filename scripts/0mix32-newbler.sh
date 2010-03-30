source ../0parameters.sh
source ../0mix32-parameters.sh
newAssembly run &> log1
cd run
addRun $r4541 &> log2
addRun $r4542 &> log3
addRun $r4543 &> log4
addRun $r4544 &> log5
addRun $r4545 &> log6
runProject &> log7
print-latex.sh $ref assembly/454AllContigs.fna Newbler
