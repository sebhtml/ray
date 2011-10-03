. ../scripts/load-modules.sh

CODE=../code

mpic++ $(find ../code/ -name "*.cpp"|grep -v main)  sff_to_fasta_main.cpp -O3 -o sff_to_fasta -I$CODE
