CXXFLAGS=-Icode -Wall -O3 

MPICC=mpic++
MPIRUN=mpirun

TARGETS=Ray


all: $(TARGETS)

OBJECTS= code/Machine.o code/common_functions.o code/Loader.o code/Read.o code/MyAllocator.o \
code/SffLoader.o code/Parameters.o code/Vertex.o code/ReadAnnotation.o code/CoverageDistribution.o \
code/Message.o code/Direction.o code/PairedRead.o code/ColorSpaceDecoder.o code/ColorSpaceLoader.o \
code/VertexLinkedList.o code/BubbleTool.o code/VerticesExtractor.o code/MessageProcessor.o \
code/SequencesLoader.o code/Chooser.o code/OpenAssemblerChooser.o code/ErrorSimulator.o \
code/BufferedData.o code/DistributionData.o code/SequencesIndexer.o code/TipWatchdog.o \
code/RepeatedVertexWatchdog.o code/SeedExtender.o code/MyForest.o code/EdgesExtractor.o code/TimePrinter.o

%.o: %.cpp
	$(MPICC) $(CXXFLAGS) -c $< -o $@

simtools: Ray-SimulateFragments Ray-SimulateErrors Ray-SimulatePairedReads

Ray-SimulatePairedReads: code/simulate_paired_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

Ray-SimulateErrors: code/simulateErrors_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

Ray-SimulateFragments: code/simulate_fragments_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@


Ray: code/ray_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@

test: code/test_main.o $(OBJECTS)
	$(MPICC) $(CXXFLAGS) $^ -o $@
	$(MPIRUN) ./test
clean:
	rm -f $(OBJECTS) $(TARGETS) *.o

