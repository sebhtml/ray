# use Ray to assemble 4 libraries
ref=/home/boiseb01/nuccore/Mycoplasma_agalactiae_PG2.fasta
readLength=50
Ray-SimulatePairedReads $ref 200 25 $readLength
Ray-SimulatePairedReads $ref 800 25 $readLength
Ray-SimulatePairedReads $ref 20000 25 $readLength

