
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

time ./graph_main 4096 debruijn
