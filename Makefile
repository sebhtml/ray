# -DDEBUG for debug information
# -DSHOW_STATISTICS for statistics on messages.

CXXFLAGS=  -Wall  -I.  -O3 -DSHOW_STATISTICS

MPIRUN=~/software/openmpi-1.4.1/output/bin/mpirun 
MPICC=~/software/openmpi-1.4.1/output/bin/mpic++

all: Ray

OBJECTS= Machine.o common_functions.o Loader.o ray_main.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o ReadAnnotation.o CoverageDistribution.o Message.o  Direction.o

%.o: %.cpp
	$(MPICC) $(CXXFLAGS) -c $< -o $@

Ray: ray_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o Ray
