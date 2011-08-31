CODE=../code
mpicxx -g $CODE/memory/malloc_types.cpp \
$CODE/communication/MessagesHandler.cpp \
$CODE/communication/Message.cpp \
$CODE/structures/StaticVector.cpp \
$CODE/communication/mpi_tags.cpp \
$CODE/memory/allocator.cpp \
$CODE/memory/RingAllocator.cpp \
$CODE/core/OperatingSystem.cpp \
$CODE/structures/Direction.cpp $CODE/structures/Vertex.cpp $CODE/structures/ReadAnnotation.cpp test_kmer.cpp $CODE/core/common_functions.cpp $CODE/structures/Kmer.cpp $CODE/cryptography/crypto.cpp  -I. -D MAXKMERLENGTH=32 -DASSERT -I$CODE -I..
./a.out TGAAATGGAAATGGTCTGGGACG

