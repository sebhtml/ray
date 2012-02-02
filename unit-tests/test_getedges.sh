. ../scripts/load-modules.sh

mpicxx -g \
 ../RayPlatform/*/*.cpp \
../code/plugin_SeedExtender/Direction.cpp ../code/plugin_VerticesExtractor/Vertex.cpp ../code/plugin_SequencesIndexer/ReadAnnotation.cpp \
 test_kmer.cpp ../code/application_core/common_functions.cpp ../code/plugin_KmerAcademyBuilder/Kmer.cpp \
 -I. -D MAXKMERLENGTH=32 -DASSERT -I../code -I ../RayPlatform -I ..
./a.out TGAAATGGAAATGGTCTGGGACG

