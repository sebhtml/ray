source ../0parameters.sh
source ../0mix13-parameters.sh
velveth velvet $wordSize -short -fasta $r4541 $r4542 $r4543 
velvetg velvet -exp_cov auto -cov_cutoff auto 
ln -s velvet/contigs.fa Assembly.fasta
ln -s $ref Reference.fasta
echo Velvet>Assembler.txt
