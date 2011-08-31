CODE=../code

mpicxx -I. $CODE/memory/malloc_types.cpp \
$CODE/communication/MessagesHandler.cpp \
$CODE/communication/Message.cpp \
$CODE/structures/StaticVector.cpp \
$CODE/communication/mpi_tags.cpp \
$CODE/memory/allocator.cpp \
$CODE/memory/RingAllocator.cpp \
 -I$CODE $CODE/core/common_functions.cpp test_invert.cpp -g $CODE/cryptography/crypto.cpp -DMAXKMERLENGTH=32 $CODE/structures/Kmer.cpp $CODE/core/OperatingSystem.cpp  -I ..
./a.out
