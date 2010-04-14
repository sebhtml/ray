source ../0parameters.sh
source ../0sim8-parameters.sh
velveth velvet $wordSize -short -fasta $left $right 
velvetg velvet -exp_cov auto -cov_cutoff auto
ln -s $ref Reference.fasta
echo Velvet > Assembler.txt
ln -s velvet/contigs.fa Assembly.fasta
