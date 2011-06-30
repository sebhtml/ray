CODE=../code
g++ $CODE/structures/Direction.cpp  $CODE/structures/ReadAnnotation.cpp $CODE/memory/malloc_types.cpp $CODE/structures/Vertex.cpp test_uniform.cpp $CODE/core/common_functions.cpp $CODE/structures/Kmer.cpp $CODE/cryptography/crypto.cpp  -I. -I$CODE -D MAXKMERLENGTH=64 -DASSERT -I.. -O3
./a.out 
