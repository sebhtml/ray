
g++ -I ../code -Wall \
 -g \
graph_main.cpp \
../code/routing/ConnectionGraph.cpp \
../code/core/statistics.cpp \
../code/routing/GraphImplementation.cpp \
../code/routing/GraphImplementationGroup.cpp \
../code/routing/GraphImplementationRandom.cpp \
../code/routing/GraphImplementationDeBruijn.cpp \
../code/routing/GraphImplementationKautz.cpp \
../code/routing/GraphImplementationExperimental.cpp \
../code/routing/GraphImplementationComplete.cpp \
-o graph_main

echo "Done compiling !"

time ./graph_main 512 debruijn 8 1
#time ./graph_main 512 debruijn 1
#time ./graph_main 511 debruijn 1
#time ./graph_main 320 kautz 1
#time ./graph_main 256 complete 1
#time ./graph_main 128 random 1
#time ./graph_main 200 group 1
