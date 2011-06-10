cd ..
g++ -g   structures/Vertex.cpp tests/test_kmer.cpp core/common_functions.cpp structures/Kmer.cpp cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=32 -DASSERT
./a.out TGAAATGGAAATGGTCTGGGACG

