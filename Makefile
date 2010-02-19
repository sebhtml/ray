# -DDEBUG for debug information
# _DDEBUG1 to show message in the extension
# -DSHOW_STATISTICS for statistics on messages.
# -DDEBUG_BARRIERS to debug barriers.

CXXFLAGS=  -Wall  -I.  -O3 -DSHOW_STATISTICS

MPICC=~/software/openmpi-1.4.1/output/bin/mpic++

all: Ray

OBJECTS= Machine.o common_functions.o Loader.o ray_main.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o ReadAnnotation.o CoverageDistribution.o Message.o  Direction.o

%.o: %.cpp
	$(MPICC) $(CXXFLAGS) -c $< -o $@

Ray: ray_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o Ray
