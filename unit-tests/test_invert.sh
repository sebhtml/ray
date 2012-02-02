. ../scripts/load-modules.sh

CODE=../code

mpicxx \
../RayPlatform/*/*.cpp \
 -I../code ../code/application_core/common_functions.cpp test_invert.cpp -g -DMAXKMERLENGTH=32 \
../code/plugin_KmerAcademyBuilder/Kmer.cpp  -I .. -I ../RayPlatform

./a.out
