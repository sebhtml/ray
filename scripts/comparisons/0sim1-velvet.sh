source ../0sim1-parameters.sh
source ../0parameters.sh
velveth velvet $wordSize -fasta -short $reads 
velvetg velvet -cov_cutoff auto -exp_cov auto 
ln -s velvet/contigs.fa Assembly.fasta
ln -s $ref Reference.fasta
echo Velvet>Assembler.txt
