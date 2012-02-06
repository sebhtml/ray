mpicxx  ../code/plugin_SeedExtender/Direction.cpp  \
../code/plugin_SequencesIndexer/ReadAnnotation.cpp \
../code/plugin_VerticesExtractor/Vertex.cpp test_kmer.cpp ../code/application_core/common_functions.cpp \
../code/plugin_KmerAcademyBuilder/Kmer.cpp   -I.  -D MAXKMERLENGTH=50 -DASSERT -I.. -I ../code -I ../RayPlatform \
../RayPlatform/*/*.cpp 

./a.out \
TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
