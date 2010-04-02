source ../0mix11-parameters.sh
source ../0parameters.sh
source ../0mix13-parameters.sh
abyss-pe k=$wordSize n=50  name=test lib="lib1 lib2" lib1="$p1left $p1right" lib2="$p2left $p2right" se="$r4541 $r4542 $r4543" &> log1
print-latex.sh $ref test-contigs.fa ABySS
