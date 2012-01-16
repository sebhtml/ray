VERSION = 2
PATCHLEVEL = 0
SUBLEVEL = 0
EXTRAVERSION = -beta4
NAME = Dark Astrocyte of Knowledge

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
# see "Is there a downside to leaving in debug symbols in release builds?"
# http://stackoverflow.com/questions/5569644/is-there-a-downside-to-leaving-in-debug-symbols-in-release-builds
# in short: the executable is larger, but symbols are in a different section (thus the code is not slower)
DEBUG = n

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
CXXFLAGS = -I code/Application -I code/Platform

# optimization
CXXFLAGS-$(OPTIMIZE) += -O3

ifeq ($(INTEL_COMPILER),n)
# g++ options
ifeq ($(uname_S),Linux)
	CXXFLAGS +=  -Wall -ansi  #-std=c++98
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

######## <Platform>

#memory
obj-y += code/Platform/memory/ReusableMemoryStore.o code/Platform/memory/MyAllocator.o code/Platform/memory/RingAllocator.o \
code/Platform/memory/malloc_types.o 
obj-y += code/Platform/memory/allocator.o
obj-y += code/Platform/memory/DefragmentationGroup.o code/Platform/memory/ChunkAllocatorWithDefragmentation.o code/Platform/memory/DefragmentationLane.o

# routing stuff for option -route-messages
#
obj-y += code/Platform/routing/GraphImplementation.o
obj-y += code/Platform/routing/GraphImplementationRandom.o
obj-y += code/Platform/routing/GraphImplementationComplete.o
obj-y += code/Platform/routing/GraphImplementationDeBruijn.o
obj-y += code/Platform/routing/GraphImplementationKautz.o
obj-y += code/Platform/routing/GraphImplementationExperimental.o
obj-y += code/Platform/routing/GraphImplementationGroup.o
obj-y += code/Platform/routing/ConnectionGraph.o

#communication
obj-y += code/Platform/communication/mpi_tags.o code/Platform/communication/VirtualCommunicator.o code/Platform/communication/BufferedData.o \
code/Platform/communication/Message.o  code/Platform/communication/MessagesHandler.o
obj-y += code/Platform/communication/MessageRouter.o

# scheduling stuff
obj-y += code/Platform/scheduling/VirtualProcessor.o
obj-y += code/Platform/scheduling/TaskCreator.o
obj-y += code/Platform/scheduling/SwitchMan.o
obj-y += code/Platform/scripting/ScriptEngine.o

#core
obj-y += code/Platform/core/slave_modes.o 
obj-y += code/Platform/core/OperatingSystem.o
obj-y += code/Platform/core/master_modes.o
obj-y += code/Platform/core/ComputeCore.o

obj-y +=  code/Platform/structures/BloomFilter.o
obj-y += code/Platform/structures/StaticVector.o 

obj-y += code/Platform/profiling/Profiler.o
obj-y += code/Platform/profiling/Derivative.o
obj-y += code/Platform/profiling/TickLogger.o
obj-y += code/Platform/profiling/TimePrinter.o

# handlers

obj-y += code/Platform/handlers/SlaveModeHandler.o
obj-y += code/Platform/handlers/MasterModeHandler.o
obj-y += code/Platform/handlers/MessageTagHandler.o



######## </Platform>


######## <Application>

# communication
obj-y += code/Application/communication/NetworkTest.o
obj-y += code/Application/communication/MessageProcessor.o

# search engine
obj-y += code/Application/search-engine/Searcher.o
obj-y += code/Application/search-engine/SearchDirectory.o
obj-y += code/Application/search-engine/ContigSearchEntry.o

#formats
obj-y += code/Application/format/ColorSpaceDecoder.o code/Application/format/ColorSpaceLoader.o code/Application/format/FastaLoader.o \
code/Application/format/FastqLoader.o code/Application/format/SffLoader.o \
code/Application/format/Amos.o

#core

obj-y += code/Application/core/Machine.o 
obj-y += code/Application/core/MachineHelper.o
obj-y += code/Application/core/Parameters.o code/Application/core/common_functions.o
obj-y += code/Application/core/statistics.o

#compression

obj-$(HAVE_LIBBZ2) += code/Application/compression/BzReader.o  

#cryptography
obj-y += code/Application/cryptography/crypto.o
obj-$(HAVE_LIBBZ2) += code/Application/format/FastqBz2Loader.o 
obj-$(HAVE_LIBZ) += code/Application/format/FastqGzLoader.o 

#graph
obj-y += code/Application/graph/GridTable.o code/Application/graph/GridTableIterator.o code/Application/graph/CoverageDistribution.o 
obj-y += code/Application/graph/CoverageGatherer.o code/Application/graph/KmerAcademy.o code/Application/graph/KmerAcademyIterator.o

#structures
obj-y += code/Application/structures/Kmer.o \
code/Application/structures/ArrayOfReads.o  code/Application/structures/Direction.o \
 code/Application/structures/PairedRead.o code/Application/structures/ReadAnnotation.o code/Application/structures/Read.o  \
code/Application/structures/Vertex.o
obj-y += code/Application/structures/AssemblySeed.o

#scaffolder
obj-y += code/Application/scaffolder/Scaffolder.o 
obj-y += code/Application/scaffolder/ScaffoldingLink.o
obj-y += code/Application/scaffolder/SummarizedLink.o
obj-y += code/Application/scaffolder/ScaffoldingAlgorithm.o
obj-y += code/Application/scaffolder/ScaffoldingVertex.o
obj-y += code/Application/scaffolder/ScaffoldingEdge.o

obj-y += code/Application/pairs/LibraryPeakFinder.o

#assembler
obj-y += code/Application/assembler/VertexMessenger.o \
code/Application/assembler/ReadFetcher.o code/Application/assembler/LibraryWorker.o code/Application/assembler/IndexerWorker.o  \
code/Application/assembler/SeedWorker.o code/Application/assembler/ExtensionElement.o \
code/Application/assembler/DepthFirstSearchData.o code/Application/assembler/SeedingData.o \
code/Application/assembler/FusionData.o code/Application/assembler/Library.o code/Application/assembler/Loader.o \
code/Application/assembler/SeedExtender.o code/Application/assembler/SequencesIndexer.o \
code/Application/assembler/SequencesLoader.o \
 code/Application/assembler/VerticesExtractor.o \
code/Application/assembler/ray_main.o code/Application/assembler/ExtensionData.o 
obj-y += code/Application/assembler/KmerAcademyBuilder.o
obj-y += code/Application/assembler/EdgePurger.o code/Application/assembler/EdgePurgerWorker.o
obj-y += code/Application/assembler/Partitioner.o
obj-y += code/Application/assembler/FusionWorker.o 
obj-y += code/Application/assembler/FusionTaskCreator.o
obj-y += code/Application/assembler/JoinerWorker.o 
obj-y += code/Application/assembler/JoinerTaskCreator.o

# heuristics
obj-y += code/Application/heuristics/BubbleTool.o code/Application/heuristics/Chooser.o code/Application/heuristics/OpenAssemblerChooser.o \
 code/Application/heuristics/TipWatchdog.o code/Application/heuristics/NovaEngine.o


######## </Application>

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
