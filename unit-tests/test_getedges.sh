CODE=../code
g++ -g $CODE/memory/malloc_types.cpp \
$CODE/structures/Direction.cpp $CODE/structures/Vertex.cpp $CODE/structures/ReadAnnotation.cpp test_kmer.cpp $CODE/core/common_functions.cpp $CODE/structures/Kmer.cpp $CODE/cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=32 -DASSERT -I$CODE -I..
./a.out TGAAATGGAAATGGTCTGGGACG

