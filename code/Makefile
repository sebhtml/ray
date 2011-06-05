# author: Sébastien Boisvert
# use make -j 30 to use 30 CPUs to compile

MAXKMERLENGTH=31
MPICXX=mpic++
CXXFLAGS=-Wall -I.

# if you use Intel's mpiicpc, uncomment the following lines
#MPICXX=mpiicpc 
#CXXFLAGS+= -DMPICH_IGNORE_CXX_SEEK -DMPICH_SKIP_MPICXX

# optimization
CXXFLAGS+= -O3 -fomit-frame-pointer

#maximum k-mer length
CXXFLAGS+= -D MAXKMERLENGTH=$(MAXKMERLENGTH)

# compile assertions
CXXFLAGS+= -DASSERT 

#compile with libz
CXXFLAGS+=-DHAVE_LIBZ
LDFLAGS+= -lz

# pack data in memory to save space
# CXXFLAGS+=-DFORCE_PACKING

#compile with libbz2
CXXFLAGS+= -DHAVE_LIBBZ2 
LDFLAGS+= -lbz2

#compile with real-time linux
CXXFLAGS+= -DHAVE_CLOCK_GETTIME 
LDFLAGS+= -lrt

TARGET=Ray

#memory
OBJS+=memory/OnDiskAllocator.o memory/ReusableMemoryStore.o memory/MyAllocator.o memory/RingAllocator.o \
memory/malloc_types.o 

#communication
OBJS+=communication/mpi_tags.o communication/VirtualCommunicator.o communication/BufferedData.o \
communication/Message.o communication/MessageProcessor.o communication/MessagesHandler.o

#formats
OBJS+=format/ColorSpaceDecoder.o format/ColorSpaceLoader.o format/FastaLoader.o \
format/FastqBz2Loader.o format/FastqGzLoader.o format/FastqLoader.o format/SffLoader.o 

#core
OBJS+=core/slave_modes.o core/Machine.o core/Parameters.o core/common_functions.o

#compression
OBJS+=compression/BzReader.o  

#cryptography
OBJS+=cryptography/crypto.o

#graph
OBJS+=graph/GridTable.o graph/VertexTable.o graph/GridTableIterator.o graph/CoverageDistribution.o 

#structures
OBJS+=structures/VertexData.o structures/Kmer.o structures/MyForestIterator.o structures/MyForest.o \
structures/ArrayOfReads.o  structures/Direction.o structures/PairedRead.o structures/ReadAnnotation.o structures/Read.o  \
structures/StaticVector.o structures/Vertex.o

#scaffolder
OBJS+=scaffolder/Scaffolder.o 

#unclassified
OBJS+=VertexMessenger.o \
ReadFetcher.o LibraryWorker.o IndexerWorker.o  \
SeedWorker.o ExtensionElement.o \
DepthFirstSearchData.o MemoryConsumptionReducer.o  SeedingData.o \
BubbleTool.o Chooser.o EarlyStoppingTechnology.o  FusionData.o Library.o Loader.o \
OpenAssemblerChooser.o SeedExtender.o SequencesIndexer.o SequencesLoader.o TimePrinter.o TipWatchdog.o VerticesExtractor.o  ray_main.o ExtensionData.o 

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
