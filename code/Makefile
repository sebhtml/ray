# author: Sébastien Boisvert
# use make -j 30 to use 30 CPUs to compile

#############################################
# compilation options

# maximum k-mer length.
MAXKMERLENGTH=64

# support for .gz files
# needs libz
# set to no if you don't have libz
HAVE_LIBZ=yes

# support for .bz2 files
# needs libbz2
# set to no if you don't have libbz2
HAVE_LIBBZ2=yes

# a real-time OS is available
# (for the function clock_gettime)
HAVE_CLOCK_GETTIME=yes

# use Intel's compiler
INTEL_COMPILER=no

# pack structures to reduce memory usage
# will work on x86 and x86_64
# won't work on Itanium and on Sparc
# The operating system's kernel must also allow the retrieval of 8-byte chunks sitting
# between two memory cache pages. Linux can do that, but I don't know for others.
FORCE_PACKING=yes

# compile assertions
# Ray may be faster when ASSERT=no
ASSERT=yes

# end of compilation options
#############################################

# mpic++ from an MPI implementation must be reachable with the PATH
# tested implementations of MPI include Open-MPI and MPICH2
MPICXX=mpic++
CXXFLAGS=-Wall -I.

# optimization
CXXFLAGS+= -O3 -fomit-frame-pointer

# if you use Intel's mpiicpc, uncomment the following lines
ifeq ($(INTEL_COMPILER),yes)
	MPICXX=mpiicpc 
	CXXFLAGS+= -DMPICH_IGNORE_CXX_SEEK -DMPICH_SKIP_MPICXX
endif

#maximum k-mer length
CXXFLAGS+= -D MAXKMERLENGTH=$(MAXKMERLENGTH)

# compile assertions
ifeq ($(ASSERT),yes)
	CXXFLAGS+= -DASSERT 
endif

#compile with libz
ifeq ($(HAVE_LIBZ),yes)
	CXXFLAGS+=-DHAVE_LIBZ
	LDFLAGS+= -lz
endif

# pack data in memory to save space
ifeq ($(FORCE_PACKING),yes)
	CXXFLAGS+=-DFORCE_PACKING
endif

#compile with libbz2
ifeq ($(HAVE_LIBBZ2),yes)
	CXXFLAGS+= -DHAVE_LIBBZ2 
	LDFLAGS+= -lbz2
endif

#compile with real-time linux
ifeq ($(HAVE_CLOCK_GETTIME),yes)
	CXXFLAGS+= -DHAVE_CLOCK_GETTIME 
	LDFLAGS+= -lrt
endif


TARGET=Ray

#memory
OBJS+=memory/OnDiskAllocator.o memory/ReusableMemoryStore.o memory/MyAllocator.o memory/RingAllocator.o \
memory/malloc_types.o 

#communication
OBJS+=communication/mpi_tags.o communication/VirtualCommunicator.o communication/BufferedData.o \
communication/Message.o communication/MessageProcessor.o communication/MessagesHandler.o

#formats
OBJS+=format/ColorSpaceDecoder.o format/ColorSpaceLoader.o format/FastaLoader.o \
format/FastqLoader.o format/SffLoader.o \
format/Amos.o

ifeq ($(HAVE_LIBBZ2),yes)
	OBJS+=format/FastqBz2Loader.o 
endif

ifeq ($(HAVE_LIBZ),yes)
	OBJS+=format/FastqGzLoader.o 
endif

#core
OBJS+=core/slave_modes.o core/Machine.o core/Parameters.o core/common_functions.o

#compression

ifeq ($(HAVE_LIBBZ2),yes)
	OBJS+=compression/BzReader.o  
endif

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
