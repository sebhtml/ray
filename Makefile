CXXFLAGS=-I. -Wall -O3  #-pedantic  -std=c++98 
#CXXFLAGS=-g -I. -DDEBUG

MPICC=mpic++

TARGETS=Ray

all: $(TARGETS)

OBJECTS= Machine.o common_functions.o Loader.o Read.o MyAllocator.o SffLoader.o Parameters.o Vertex.o ReadAnnotation.o CoverageDistribution.o Message.o  Direction.o  PairedRead.o ColorSpaceDecoder.o ColorSpaceLoader.o VertexLinkedList.o BubbleTool.o VerticesExtractor.o MessageProcessor.o SequencesLoader.o Chooser.o OpenAssemblerChooser.o  ErrorSimulator.o BufferedData.o DistributionData.o SequencesIndexer.o TipWatchdog.o RepeatedVertexWatchdog.o SeedExtender.o MyForest.o EdgesExtractor.o TimePrinter.o

%.o: %.cpp
	@echo MPICC $<
	@$(MPICC) $(CXXFLAGS) -c $< -o $@

simtools: Ray-SimulateFragments Ray-SimulateErrors Ray-SimulatePairedReads


Ray-SimulatePairedReads: simulate_paired_main.o $(OBJECTS)
	@echo A.OUT $@
	@$(MPICC) $(CXXFLAGS) $^ -o $@

Ray-SimulateErrors: simulateErrors_main.o $(OBJECTS)
	@echo A.OUT $@
	@$(MPICC) $(CXXFLAGS) $^ -o $@

Ray-SimulateFragments: simulate_fragments_main.o $(OBJECTS)
	@echo A.OUT $@
	@$(MPICC) $(CXXFLAGS) $^ -o $@


Ray: ray_main.o $(OBJECTS)
	@echo A.OUT $@
	@$(MPICC) $(CXXFLAGS) $^ -o $@

test: test_main.o $(OBJECTS)
	@echo TEST
	@$(MPICC) $(CXXFLAGS) $^ -o $@
	@$(MPIRUN) ./test
clean:
	@echo CLEAN
	@rm -f $(OBJECTS) $(TARGETS) *.o

