cd ..
g++ tests/test_kmer.cpp core/common_functions.cpp structures/Kmer.cpp cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=50 -DASSERT
./a.out \
TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
