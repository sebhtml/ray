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
# DEBUG for debug information
# DEBUG1 to show message in the extension
# SHOW_STATISTICS for statistics on messages.
# WRITE_COVERAGE_DISTRIBUTION to write coverage distribution.
# ASSERT verify various assertions.
# SHOW_PROGRESSION to show progression.

CXXFLAGS=  -Wall  -I.  -O3 

MPICC=mpic++

all: Ray

OBJECTS= Machine.o common_functions.o Loader.o ray_main.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o ReadAnnotation.o CoverageDistribution.o Message.o  Direction.o

%.o: %.cpp
	$(MPICC) $(CXXFLAGS) -c $< -o $@

Ray: ray_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o Ray
