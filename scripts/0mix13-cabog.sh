source ../0parameters.sh
source ../0mix13-parameters.sh
convert-fasta-to-v2.pl -454 -l 1 -s $r4541 -q $r4541qual > 1.frg
convert-fasta-to-v2.pl -454 -l 2 -s $r4542 -q $r4542qual > 2.frg
convert-fasta-to-v2.pl -454 -l 3 -s $r4543 -q $r4543qual > 3.frg
runCA -d cabog -p cabog unitigger=bog and overlapper=mer  1.frg 2.frg 3.frg
ln -s  cabog/9-terminator/cabog.ctg.fasta Assembly.fasta
echo CABOG > Assembler.txt
ln -s $ref Reference.fasta
