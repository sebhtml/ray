source ../0sim4-parameters.sh
source ../0parameters.sh
velveth velvet $wordSize -long -fasta $reads 
velvetg velvet -exp_cov auto -cov_cutoff auto 
echo Velvet>Assembler.txt
ln -s $ref Reference.fasta
ln -s velvet/contigs.fa Assembly.fasta
