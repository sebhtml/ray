source ../0sim5-parameters.sh
source ../0parameters.sh
velveth velvet $wordSize -short -fasta $left $right 
velvetg velvet -exp_cov auto -cov_cutoff auto 
echo Velvet>Assembler.txt
ln -s $ref Reference.fasta
ln -s velvet/contigs.fa Assembly.fasta
