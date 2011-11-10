
g++ -I ../code -Wall -O3 \
graph_main.cpp \
../code/routing/ConnectionGraph.cpp \
../code/core/statistics.cpp \
-o graph_main

./graph_main 4096
