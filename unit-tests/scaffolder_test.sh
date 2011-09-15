
g++ scaffolder_test.cpp \
 ../code/scaffolder/ScaffoldingVertex.cpp \
 ../code/scaffolder/ScaffoldingEdge.cpp \
 ../code/scaffolder/ScaffoldingAlgorithm.cpp \
 -O3 -o scaffolderTest -I ../code -I .. \
-Wall

./scaffolderTest vertices.txt edges.txt
