cd ..
g++ -I. core/common_functions.cpp tests/test_invert.cpp -g cryptography/crypto.cpp -DMAXKMERLENGTH=32 structures/Kmer.cpp
./a.out
