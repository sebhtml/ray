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

# You can set compile time options too:
# -DSHOW_EXTEND_WITH_SEED (show context-dependant extension, when using seeds)
# -DSHOW_SENT_MESSAGES (show communication statistics)
# -DSHOW_PROGRESS (show on-screen progress)
# -DSHOW_MINI_GRAPH (show graphviz output for tips and bubbles)
# -DSHOW_FILTER (show the internals of the filters for 1-coverage vertices)
# -DDEBUG (run an array of assert(<.>) during execution)
# -DSHOW_TRON (show Tron internals)
# -O3 (maximum optimization)
# -g (debug code for gdb utilization)

# debug options follow:
CXXFLAGS=-Wall  -I. -O3  -g -DSHOW_SENT_MESSAGES -DSHOW_PROGRESS -DSHOW_FILTER -DDEBUG -DSHOW_TIP_LOST -DSHOW_CHOICE  -DSHOW_TRON

# production options follow:
#CXXFLAGS=-I. -Wall -O3  

# the default is to use mpic++ provided in your $PATH
MPICC=mpic++
MPIRUN=mpirun

TARGETS=Ray

all: $(TARGETS)

OBJECTS= Machine.o common_functions.o Loader.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o ReadAnnotation.o CoverageDistribution.o Message.o  Direction.o  PairedRead.o ColorSpaceDecoder.o ColorSpaceLoader.o VertexLinkedList.o BubbleTool.o VerticesExtractor.o MessageProcessor.o SequencesLoader.o Chooser.o OpenAssemblerChooser.o TronChooser.o

%.o: %.cpp
	@echo MPICC $<
	@$(MPICC) $(CXXFLAGS) -c $< -o $@

SimulatePairedReads: simulate_paired_main.o $(OBJECTS)
	$(CC) $(CXXFLAGS) $^ -o $@

SimulateErrors: simulateErrors_main.o $(OBJECTS)
	$(CC) $(CXXFLAGS) $^ -o $@

SimulateFragments: simulate_fragments_main.o $(OBJECTS)
	$(CC) $(CXXFLAGS) $^ -o $@


Ray: ray_main.o $(OBJECTS)
	@echo MPICC Ray
	@$(MPICC) $(CXXFLAGS) $^ -o $@

test: test_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@
	$(MPIRUN) ./test
clean:
	@echo CLEAN
	@rm -f $(OBJECTS) $(TARGETS)

