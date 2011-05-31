# author: Sébastien Boisvert
# use make -j 30 to use 30 CPUs to compile


MPICXX=mpic++
CXXFLAGS=-Wall -Icode

# if you use Intel's mpiicpc, uncomment the following lines
#MPICXX=mpiicpc 
#CXXFLAGS+= -DMPICH_IGNORE_CXX_SEEK -DMPICH_SKIP_MPICXX

# optimization
CXXFLAGS+= -O3 -fomit-frame-pointer

# compile assertions
CXXFLAGS+= -DASSERT 

#compile with libz
CXXFLAGS+=-DHAVE_LIBZ
LDFLAGS+= -lz

#compile with libbz2
CXXFLAGS+= -DHAVE_LIBBZ2 
LDFLAGS+= -lbz2

#compile with real-time linux
CXXFLAGS+= -DHAVE_CLOCK_GETTIME 
LDFLAGS+= -lrt

TARGET=code/Ray

OBJS=code/VertexMessenger.o code/OnDiskAllocator.o code/mpi_tags.o code/ReadFetcher.o code/LibraryWorker.o code/IndexerWorker.o code/GridTable.o code/VertexTable.o code/VertexData.o code/GridTableIterator.o code/ReusableMemoryStore.o code/SeedWorker.o code/ExtensionElement.o code/VirtualCommunicator.o code/DepthFirstSearchData.o code/MemoryConsumptionReducer.o  code/MyForestIterator.o code/SeedingData.o code/ArrayOfReads.o code/BubbleTool.o code/BufferedData.o code/BzReader.o code/Chooser.o code/ColorSpaceDecoder.o code/ColorSpaceLoader.o code/common_functions.o code/CoverageDistribution.o code/Direction.o code/EarlyStoppingTechnology.o  code/FastaLoader.o code/FastqBz2Loader.o code/FastqGzLoader.o code/FastqLoader.o code/FusionData.o code/Library.o code/Loader.o code/Machine.o code/Message.o code/MessageProcessor.o code/MessagesHandler.o code/MyAllocator.o code/MyForest.o code/OpenAssemblerChooser.o code/PairedRead.o code/Parameters.o code/ReadAnnotation.o code/Read.o code/RingAllocator.o code/SeedExtender.o code/SequencesIndexer.o code/SequencesLoader.o code/SffLoader.o code/StaticVector.o code/TimePrinter.o code/TipWatchdog.o code/Vertex.o code/VertexLinkedList.o code/VerticesExtractor.o  code/ray_main.o code/crypto.o code/ExtensionData.o code/malloc_types.o code/slave_modes.o code/Scaffolder.o

# inference rule
%.o: %.cpp
	$(MPICXX) -c -o $@ $<  $(CXXFLAGS)

# the target is Ray
all: $(TARGET)

# how to make Ray
$(TARGET): $(OBJS)
	$(MPICXX) $(LDFLAGS) $(OBJS) -o $@
	@echo "Ray executable is $(TARGET)"

clean:
	rm -f $(TARGET) $(OBJS)
