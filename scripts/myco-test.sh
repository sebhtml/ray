# use Ray to assemble 4 libraries
ref=/home/boiseb01/nuccore/Mycoplasma_agalactiae_PG2.fasta
Ray-SimulatePairedReads $ref 200 12 21
Ray-SimulatePairedReads $ref 500 12 21
Ray-SimulatePairedReads $ref 1000 12 21
Ray-SimulatePairedReads $ref 4000 12 21

