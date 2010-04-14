source ../0parameters.sh
source ../0mix32-parameters.sh
velveth velvet $wordSize -short -fasta $r4541 $r4542 $r4543 $r4544 $r4545 
velvetg velvet -exp_cov auto -cov_cutoff auto
echo Velvet>Assembler.txt
ln -s $ref Reference.fasta
ln -s velvet/contigs.fa Assembly.fasta
