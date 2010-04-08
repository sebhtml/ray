# use Ray to assemble 4 libraries
ref=/home/boiseb01/nuccore/Mycoplasma_agalactiae_PG2.fasta
readLength=50
Ray-SimulatePairedReads $ref 200 12 $readLength
Ray-SimulatePairedReads $ref 500 12 $readLength
Ray-SimulatePairedReads $ref 1000 12 $readLength
Ray-SimulatePairedReads $ref 4000 12 $readLength

mpirun -np 16 Ray \
LoadPairedEndReads Mycop,1000b,2x50b,12X_1.fasta Mycop,1000b,2x50b,12X_2.fasta 1000 5 \
LoadPairedEndReads Mycop,200b,2x50b,12X_1.fasta   Mycop,200b,2x50b,12X_2.fasta 200 5 \
LoadPairedEndReads Mycop,4000b,2x50b,12X_1.fasta  Mycop,4000b,2x50b,12X_2.fasta 4000 5 \
LoadPairedEndReads Mycop,500b,2x50b,12X_1.fasta Mycop,500b,2x50b,12X_2.fasta 500 5 
