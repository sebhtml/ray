
cd ..
g++ tests/test_kmer.cpp core/common_functions.cpp structures/Kmer.cpp cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=32 -DASSERT
./a.out TGAAATGGAAATGGTCTGGGAAG

g++ tests/test_kmer.cpp core/common_functions.cpp structures/Kmer.cpp cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=64 -DASSERT
./a.out \
TGAAATGGAAATGGTCTGGGAAAAACAACTAAAAGATATTATTGTAGTA

g++ tests/test_kmer.cpp core/common_functions.cpp structures/Kmer.cpp cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=96 -DASSERT
./a.out \
TGAAATGGAAATGGTCTGGGAAAAACAACTAAAAGATATTATTGTAGTAGCTGGTTTTGAAATTTATGACGCTGAAATAACTCCCCACTA

g++ tests/test_kmer.cpp core/common_functions.cpp structures/Kmer.cpp cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=128 -DASSERT
./a.out \
TGAAATGGAAATGGTCTGGGAAAAACAACTAAAAGATATTATTGTAGTAGCTGGTTTTGAAATTTATGACGCTGAAATAACTCCCCACTATATTTTCACCAAATTTATT

g++ tests/test_kmer.cpp core/common_functions.cpp structures/Kmer.cpp cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=75 -DASSERT
./a.out \
TGAAATGGAAATGGTCTGGGAAAAACAACTAAAAGATATTATTGTAGTAGCTGGTTTTGAAATTTATGACGCT

g++ tests/test_kmer.cpp core/common_functions.cpp structures/Kmer.cpp cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=50 -DASSERT
./a.out \
TGAAATGGAAATGGTCTGGGAAAAACAACTAAAAGATATTAT
