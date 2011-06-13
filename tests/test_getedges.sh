CODE=../code
g++ -g  $CODE/structures/Vertex.cpp test_kmer.cpp $CODE/core/common_functions.cpp $CODE/structures/Kmer.cpp $CODE/cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=32 -DASSERT -I$CODE -I..
./a.out TGAAATGGAAATGGTCTGGGACG

