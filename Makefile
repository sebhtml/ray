# DEBUG for debug information
# DEBUG1 to show message in the extension
# SHOW_STATISTICS for statistics on messages.
# DEBUG_BARRIERS to debug barriers.
# WRITE_COVERAGE_DISTRIBUTION to write coverage distribution.
# USE_ISEND  necessary with mvapich2. (autodetect)

CXXFLAGS=  -Wall  -I.  -O3  #-DSHOW_STATISTICS

MPICC=~/software/openmpi-1.4.1/output/bin/mpic++
#MPICC=/home/boiseb01/software/mvapich2/bin/mpicxx
MPICC=/home/boiseb01/software/ompi-1.4.1-gcc/bin/mpic++
MPICC=/home/boiseb01/software/mpich2-nemesis/bin/mpic++

all: Ray

OBJECTS= Machine.o common_functions.o Loader.o ray_main.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o ReadAnnotation.o CoverageDistribution.o Message.o  Direction.o

%.o: %.cpp
	$(MPICC) $(CXXFLAGS) -c $< -o $@

Ray: ray_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o Ray
