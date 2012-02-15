
g++ -I ../RayPlatform -Wall \
 -g \
graph_main.cpp \
../RayPlatform/routing/ConnectionGraph.cpp \
../RayPlatform/core/statistics.cpp \
../RayPlatform/routing/GraphImplementation.cpp \
../RayPlatform/routing/GraphImplementationGroup.cpp \
../RayPlatform/routing/GraphImplementationRandom.cpp \
../RayPlatform/routing/GraphImplementationDeBruijn.cpp \
../RayPlatform/routing/GraphImplementationKautz.cpp \
../RayPlatform/routing/GraphImplementationExperimental.cpp \
../RayPlatform/routing/GraphImplementationComplete.cpp \
-o graph_main

echo "Done compiling !"

time ./graph_main 512 debruijn 8 1
time ./graph_main 512 debruijn 2 1
#time ./graph_main 511 debruijn 1
time ./graph_main 320 kautz 1 1
#time ./graph_main 256 complete 1
#time ./graph_main 128 random 1
#time ./graph_main 200 group 1
