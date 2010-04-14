source ../0parameters.sh
source ../0mix32-parameters.sh
source ../0mix31-parameters.sh
velveth velvet $wordSize -short -fasta $r4541 $r4542 $r4543 $r4544 $r4545 -fasta -short $r1 $r2 $r3  
velvetg velvet -exp_cov 164 -cov_cutoff 10 
echo Velvet>Assembler.txt
ln -s $ref Reference.fasta
ln -s velvet/contigs.fa Assembly.fasta
