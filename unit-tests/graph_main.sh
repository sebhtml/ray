
g++ -I ../code -Wall -O3 \
graph_main.cpp \
../code/routing/ConnectionGraph.cpp \
../code/core/statistics.cpp \
../code/routing/GraphImplementation.cpp \
../code/routing/GraphImplementationGroup.cpp \
../code/routing/GraphImplementationRandom.cpp \
../code/routing/GraphImplementationDeBruijn.cpp \
../code/routing/GraphImplementationComplete.cpp \
-o graph_main

echo "Done compiling !"

time ./graph_main 4096 debruijn 0
#time ./graph_main 256 complete 1
#time ./graph_main 128 random 1
#time ./graph_main 200 group 1
