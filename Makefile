# Ray
    #Copyright (C) 2010  Sébastien Boisvert

	#http://DeNovoAssembler.SourceForge.Net/

    #This program is free software: you can redistribute it and/or modify
    #it under the terms of the GNU General Public License as published by
    #the Free Software Foundation, version 3 of the License.

    #This program is distributed in the hope that it will be useful,
    #but WITHOUT ANY WARRANTY; without even the implied warranty of
    #MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    #GNU General Public License for more details.

    #You have received a copy of the GNU General Public License
    #along with this program (gpl-3.0.txt).  
	#see <http://www.gnu.org/licenses/>


# These are things you can provide with -D<Something> for development purposes
# DEBUG verify various assertions.
# SHOW_PROGRESS to show progression, this one is useful if you want to know what is going on.
#
# detections at compilation:
#
# MPICH2_VERSION is set when using mpi.h from mpich2
#   if MPICH2_VERSION is set, then MPI_Isend is utilized for all point-to-point communications.
#   otherwise, MPI_Send is used because Open-MPI is smarter: it sends eagerly small messages!

CXXFLAGS=  -Wall  -I.  -O3     -DSHOW_PROGRESS -DDEBUG -g

# the default is to use mpic++ provided in your $PATH
MPICC=mpic++

# if you want to change the executable name, do it t here!
TARGETS=Ray #SimulateFragments SimulateErrors SimulatePairedReads

all: $(TARGETS)


OBJECTS= Machine.o common_functions.o Loader.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o ReadAnnotation.o CoverageDistribution.o Message.o  Direction.o  PairedRead.o ColorSpaceDecoder.o ColorSpaceLoader.o VertexLinkedList.o

%.o: %.cpp
	$(MPICC) $(CXXFLAGS) -c $< -o $@

SimulatePairedReads: simulate_paired_main.o $(OBJECTS)
	$(CC) $(CXXFLAGS) $^ -o $@

SimulateErrors: simulateErrors_main.o $(OBJECTS)
	$(CC) $(CXXFLAGS) $^ -o $@

SimulateFragments: simulate_fragments_main.o $(OBJECTS)
	$(CC) $(CXXFLAGS) $^ -o $@


Ray: ray_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

test: test_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@
	/home/boiseb01/software/ompi-1.4.1-gcc/bin/mpirun ./test
clean:
	rm -f $(OBJECTS) $(TARGETS)

