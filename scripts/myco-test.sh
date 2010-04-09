# use Ray to assemble 4 libraries
ref=/home/boiseb01/nuccore/Mycoplasma_agalactiae_PG2.fasta
readLength=50
Ray-SimulatePairedReads $ref 200 50 $readLength
Ray-SimulatePairedReads $ref 800 50 $readLength
Ray-SimulatePairedReads $ref 20000 50 $readLength

