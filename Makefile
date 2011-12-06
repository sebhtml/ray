VERSION = 1
PATCHLEVEL = 7
SUBLEVEL = 1
EXTRAVERSION = 
NAME = Deep Curtain of Broad Ranges

RAY_VERSION = $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)

# author: Sébastien Boisvert
# Makefile for the ray assembler
# Objects appended to obj-y are compiled and linked.
# Objects appended to obj-n are not compiled and linked.
#############################################
# compilation options

# installation prefix with make install
PREFIX=install-prefix

# maximum k-mer length
# in nucleotides
# 32 nucleotides are stored on 1x64 bits
# 45 nucleotides are stored on 2x64 bits
# 64 nucleotides are stored on 2x64 bits
# 96 nucleotides are stored on 3x64 bits
# Intermediate values should utilise intermediate numbers of bits.
# There is no maximum value for MAXKMERLENGTH
MAXKMERLENGTH = 32

# support for .gz files
# needs libz
# set to no if you don't have libz
# y/n
HAVE_LIBZ = n

# support for .bz2 files
# needs libbz2
# set to no if you don't have libbz2
# y/n
HAVE_LIBBZ2 = n

# use Intel's compiler
# the name of the Intel MPI C++ compiler is mpiicpc
# Open-MPI and MPICH2 utilise mpic++ for the name.
# y/n
INTEL_COMPILER = n

# pack structures to reduce memory usage
# will work on x86 and x86_64
# won't work on Itanium and on Sparc
# The operating system's kernel must also allow the retrieval of 8-byte chunks sitting
# between two memory cache pages. Linux can do that, but I don't know for others.
# if you are not sure, type uname -a to get your processor architecture.
#
# Seems to fail or be very slow on Intel Xeon too.
#
# y/n
FORCE_PACKING = n

# compile assertions
# Ray may be faster when ASSERT=n
# y/n
ASSERT = n

# collect profiling information with -run-profiler
# if set to n, the code is not even compiled in
CONFIG_PROFILER_COLLECT=y

# use the precision clock
# needs -l rt too
CONFIG_CLOCK_GETTIME=n

# OS detection based on git Makefile
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# optimize
OPTIMIZE = y

# add -g to gcc
DEBUG = y

# profiling
GPROF = n

ifeq ($(GPROF),y)
	OPTIMIZE = n
	FORCE_PACKING = n
endif

PEDANTIC = n

MPICXX-y = mpicxx

# mpic++ from an MPI implementation must be reachable with the PATH
# tested implementations of MPI include Open-MPI and MPICH2
CXXFLAGS = -Icode 

# optimization
CXXFLAGS-$(OPTIMIZE) += -O3

ifeq ($(INTEL_COMPILER),n)
# g++ options
ifeq ($(uname_S),Linux)
	CXXFLAGS +=  -Wall -ansi -g #-std=c++98
	CXXFLAGS-$(PEDANTIC) += -pedantic -Wextra 
endif
endif

# profiling
CXXFLAGS-$(GPROF) += -g -pg
LDFLAGS-$(GPROF) += -pg -g

# if you use Intel's mpiicpc, uncomment the following lines
MPICXX-$(INTEL_COMPILER) = mpiicpc 
CXXFLAGS-$(INTEL_COMPILER) += -DMPICH_IGNORE_CXX_SEEK -DMPICH_SKIP_MPICXX

#maximum k-mer length
CXXFLAGS-y += -D MAXKMERLENGTH=$(MAXKMERLENGTH)

# compile assertions
CXXFLAGS-$(ASSERT) += -DASSERT

#compile with libz
CXXFLAGS-$(HAVE_LIBZ) += -DHAVE_LIBZ
LDFLAGS-$(HAVE_LIBZ) += -lz

#compile with libbz2
CXXFLAGS-$(HAVE_LIBBZ2) += -DHAVE_LIBBZ2 
LDFLAGS-$(HAVE_LIBBZ2) += -lbz2

#debug flag
CXXFLAGS-$(DEBUG) += -g
LDFLAGS-$(DEBUG)  += -g

# pack data in memory to save space
CXXFLAGS-$(FORCE_PACKING) += -DFORCE_PACKING

CXXFLAGS-$(CONFIG_PROFILER_COLLECT) += -D CONFIG_PROFILER_COLLECT
CXXFLAGS-$(CONFIG_CLOCK_GETTIME) += -D CONFIG_CLOCK_GETTIME
LDFLAGS-$(CONFIG_CLOCK_GETTIME) += -l rt
CXXFLAGS-y += -D RAY_VERSION=\"$(RAY_VERSION)\"

CXXFLAGS += $(CXXFLAGS-y)
LDFLAGS += $(LDFLAGS-y)

MPICXX = $(MPICXX-y)

TARGETS=Ray

# object files

#memory
obj-y += code/memory/ReusableMemoryStore.o code/memory/MyAllocator.o code/memory/RingAllocator.o \
code/memory/malloc_types.o 
obj-y += code/memory/allocator.o
obj-y += code/memory/DefragmentationGroup.o code/memory/ChunkAllocatorWithDefragmentation.o code/memory/DefragmentationLane.o

# routing stuff for option -route-messages
#
obj-y += code/routing/GraphImplementation.o
obj-y += code/routing/GraphImplementationRandom.o
obj-y += code/routing/GraphImplementationComplete.o
obj-y += code/routing/GraphImplementationDeBruijn.o
obj-y += code/routing/GraphImplementationKautz.o
obj-y += code/routing/GraphImplementationExperimental.o
obj-y += code/routing/GraphImplementationGroup.o
obj-y += code/routing/ConnectionGraph.o

#communication
obj-y += code/communication/mpi_tags.o code/communication/VirtualCommunicator.o code/communication/BufferedData.o \
code/communication/Message.o code/communication/MessageProcessor.o code/communication/MessagesHandler.o
obj-y += code/communication/NetworkTest.o
obj-y += code/communication/MessageRouter.o


# scheduling stuff
obj-y += code/scheduling/VirtualProcessor.o
obj-y += code/scheduling/TaskCreator.o

#formats
obj-y += code/format/ColorSpaceDecoder.o code/format/ColorSpaceLoader.o code/format/FastaLoader.o \
code/format/FastqLoader.o code/format/SffLoader.o \
code/format/Amos.o

#core
obj-y += code/core/slave_modes.o code/core/Machine.o code/core/Parameters.o code/core/common_functions.o
obj-y += code/core/OperatingSystem.o
obj-y += code/core/statistics.o

#compression

obj-$(HAVE_LIBBZ2) += code/compression/BzReader.o  

#cryptography
obj-y += code/cryptography/crypto.o
obj-$(HAVE_LIBBZ2) += code/format/FastqBz2Loader.o 
obj-$(HAVE_LIBZ) += code/format/FastqGzLoader.o 

#graph
obj-y += code/graph/GridTable.o code/graph/GridTableIterator.o code/graph/CoverageDistribution.o 
obj-y += code/graph/CoverageGatherer.o code/graph/KmerAcademy.o code/graph/KmerAcademyIterator.o

#structures
obj-y += code/structures/Kmer.o \
code/structures/ArrayOfReads.o  code/structures/Direction.o code/structures/PairedRead.o code/structures/ReadAnnotation.o code/structures/Read.o  \
code/structures/StaticVector.o code/structures/Vertex.o code/structures/BloomFilter.o
obj-y += code/structures/AssemblySeed.o

#scaffolder
obj-y += code/scaffolder/Scaffolder.o 
obj-y += code/scaffolder/ScaffoldingLink.o
obj-y += code/scaffolder/SummarizedLink.o
obj-y += code/scaffolder/ScaffoldingAlgorithm.o
obj-y += code/scaffolder/ScaffoldingVertex.o
obj-y += code/scaffolder/ScaffoldingEdge.o

obj-y += code/pairs/LibraryPeakFinder.o

#assembler
obj-y += code/assembler/VertexMessenger.o \
code/assembler/ReadFetcher.o code/assembler/LibraryWorker.o code/assembler/IndexerWorker.o  \
code/assembler/SeedWorker.o code/assembler/ExtensionElement.o \
code/assembler/DepthFirstSearchData.o code/assembler/SeedingData.o \
code/assembler/FusionData.o code/assembler/Library.o code/assembler/Loader.o \
code/assembler/SeedExtender.o code/assembler/SequencesIndexer.o \
code/assembler/SequencesLoader.o \
code/assembler/TimePrinter.o code/assembler/VerticesExtractor.o \
code/assembler/ray_main.o code/assembler/ExtensionData.o 
obj-y += code/assembler/KmerAcademyBuilder.o
obj-y += code/assembler/EdgePurger.o code/assembler/EdgePurgerWorker.o
obj-y += code/assembler/Partitioner.o
obj-y += code/assembler/FusionWorker.o 
obj-y += code/assembler/FusionTaskCreator.o
obj-y += code/assembler/JoinerWorker.o 
obj-y += code/assembler/JoinerTaskCreator.o

obj-y += code/profiling/Profiler.o
obj-y += code/profiling/Derivative.o

# heuristics
obj-y += code/heuristics/BubbleTool.o code/heuristics/Chooser.o code/heuristics/OpenAssemblerChooser.o \
 code/heuristics/TipWatchdog.o code/heuristics/RayNovaEngine.o

#################################

# inference rule
#@echo "  MPICXX" $<
%.o: %.cpp
	@$(MPICXX) -c -o $@ $<  $(CXXFLAGS)
	@echo MPICXX $<

# the target is Ray
all: $(TARGETS)

showOptions: 
	@echo ""
	@echo "Compilation options (you can change them of course)"
	@echo ""
	@echo PREFIX = $(PREFIX)
	@echo MAXKMERLENGTH = $(MAXKMERLENGTH)
	@echo FORCE_PACKING = $(FORCE_PACKING)
	@echo ASSERT = $(ASSERT)
	@echo HAVE_LIBZ = $(HAVE_LIBZ)
	@echo HAVE_LIBBZ2 = $(HAVE_LIBBZ2)
	@echo INTEL_COMPILER = $(INTEL_COMPILER)
	@echo MPICXX = $(MPICXX)
	@echo GPROF = $(GPROF)
	@echo OPTIMIZE = $(OPTIMIZE)
	@echo DEBUG = $(DEBUG)
	@echo ""
	@echo "Compilation and linking flags (generated automatically)"
	@echo ""
	@echo CXXFLAGS = $(CXXFLAGS)
	@echo LDFLAGS = $(LDFLAGS)
	@echo ""
	@touch showOptions
	
# how to make Ray
Ray: showOptions $(obj-y)
	@$(MPICXX) $(LDFLAGS) $(obj-y) -o $@
	@echo MPICXX $@
	@echo $(PREFIX) > PREFIX
	@echo $(TARGETS) > TARGETS

clean:
	@rm -f $(TARGETS) $(obj-y) showOptions PREFIX TARGETS
	@echo CLEAN

install: 
	@scripts/install.sh
