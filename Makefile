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

# a real-time OS is available
# (for the function clock_gettime)
# y/n
HAVE_CLOCK_GETTIME = n

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
# y/n
FORCE_PACKING = y

# compile assertions
# Ray may be faster when ASSERT=n
# y/n
ASSERT = n

# Create VirtualNextGenSequencer
# needs boost library
VIRTUAL_SEQUENCER = n

# end of compilation options
#############################################

# OS detection based on git Makefile
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_M := $(shell sh -c 'uname -m 2>/dev/null || echo not')
uname_O := $(shell sh -c 'uname -o 2>/dev/null || echo not')
uname_R := $(shell sh -c 'uname -r 2>/dev/null || echo not')
uname_P := $(shell sh -c 'uname -p 2>/dev/null || echo not')
uname_V := $(shell sh -c 'uname -v 2>/dev/null || echo not')

ifdef MSVC
        uname_S := Windows
        uname_O := Windows
endif

# optimize
OPTIMIZE = y

# profiling
GPROF = n
DEBUG = n

ifeq ($(GPROF),y)
	OPTIMIZE = n
endif

PEDANTIC = n

MPICXX-y = mpic++

# mpic++ from an MPI implementation must be reachable with the PATH
# tested implementations of MPI include Open-MPI and MPICH2
CXXFLAGS = -Icode

# optimization
CXXFLAGS-$(OPTIMIZE) += -O3

ifeq ($(INTEL_COMPILER),n)
# g++ options
ifeq ($(uname_S),Linux)
	CXXFLAGS += -Wall -std=c++98
	#CXXFLAGS-$(OPTIMIZE) += -fomit-frame-pointer -finline-functions -funroll-loops
	CXXFLAGS-$(PEDANTIC) += -pedantic -Wextra 
endif
endif

# profiling
CXXFLAGS-$(GPROF) += -g -pg

# if you use Intel's mpiicpc, uncomment the following lines
MPICXX-$(INTEL_COMPILER) = mpiicpc 
CXXFLAGS-$(INTEL_COMPILER) += -DMPICH_IGNORE_CXX_SEEK -DMPICH_SKIP_MPICXX

MPICXX = $(MPICXX-y)

#maximum k-mer length
CXXFLAGS += -D MAXKMERLENGTH=$(MAXKMERLENGTH)

# compile assertions
CXXFLAGS-$(ASSERT) += -DASSERT

#compile with libz
CXXFLAGS-$(HAVE_LIBZ) += -DHAVE_LIBZ
LDFLAGS-$(HAVE_LIBZ) += -lz

# pack data in memory to save space
CXXFLAGS-$(FORCE_PACKING) += -DFORCE_PACKING

#compile with libbz2
CXXFLAGS-$(HAVE_LIBBZ2) += -DHAVE_LIBBZ2 
LDFLAGS-$(HAVE_LIBBZ2) += -lbz2

#debug flag
CXXFLAGS-$(DEBUG) += -g
LDFLAGS-$(DEBUG)  += -g

#compile with real-time linux
CXXFLAGS-$(HAVE_CLOCK_GETTIME) += -DHAVE_CLOCK_GETTIME 
LDFLAGS-$(HAVE_CLOCK_GETTIME) += -lrt

LDFLAGS-$(GPROF) += -pg

CXXFLAGS += $(CXXFLAGS-y)
LDFLAGS += $(LDFLAGS-y)


TARGETS-$(VIRTUAL_SEQUENCER) += readSimulator/VirtualNextGenSequencer

TARGETS=code/Ray $(TARGETS-y)

#memory
obj-y += code/memory/OnDiskAllocator.o code/memory/ReusableMemoryStore.o code/memory/MyAllocator.o code/memory/RingAllocator.o \
code/memory/malloc_types.o 
obj-y += code/memory/allocator.o

#communication
obj-y += code/communication/mpi_tags.o code/communication/VirtualCommunicator.o code/communication/BufferedData.o \
code/communication/Message.o code/communication/MessageProcessor.o code/communication/MessagesHandler.o

#formats
obj-y += code/format/ColorSpaceDecoder.o code/format/ColorSpaceLoader.o code/format/FastaLoader.o \
code/format/FastqLoader.o code/format/SffLoader.o \
code/format/Amos.o

obj-$(HAVE_LIBBZ2) += code/format/FastqBz2Loader.o 
obj-$(HAVE_LIBZ) += code/format/FastqGzLoader.o 

#core
obj-y += code/core/slave_modes.o code/core/Machine.o code/core/Parameters.o code/core/common_functions.o

#compression

obj-$(HAVE_LIBBZ2) += code/compression/BzReader.o  

#cryptography
obj-y += code/cryptography/crypto.o

#graph
obj-y += code/graph/GridTable.o code/graph/VertexTable.o code/graph/GridTableIterator.o code/graph/CoverageDistribution.o 
obj-y += code/graph/CoverageGatherer.o

#structures
obj-y += code/structures/VertexData.o code/structures/Kmer.o code/structures/MyForestIterator.o code/structures/MyForest.o \
code/structures/ArrayOfReads.o  code/structures/Direction.o code/structures/PairedRead.o code/structures/ReadAnnotation.o code/structures/Read.o  \
code/structures/StaticVector.o code/structures/Vertex.o

#scaffolder
obj-y += code/scaffolder/Scaffolder.o 

#assembler
obj-y += code/assembler/VertexMessenger.o \
code/assembler/ReadFetcher.o code/assembler/LibraryWorker.o code/assembler/IndexerWorker.o  \
code/assembler/SeedWorker.o code/assembler/ExtensionElement.o \
code/assembler/DepthFirstSearchData.o code/assembler/MemoryConsumptionReducer.o  code/assembler/SeedingData.o \
code/assembler/BubbleTool.o code/assembler/Chooser.o \
code/assembler/FusionData.o code/assembler/Library.o code/assembler/Loader.o \
code/assembler/OpenAssemblerChooser.o code/assembler/SeedExtender.o code/assembler/SequencesIndexer.o \
code/assembler/SequencesLoader.o \
code/assembler/TimePrinter.o code/assembler/TipWatchdog.o code/assembler/VerticesExtractor.o \
code/assembler/ray_main.o code/assembler/ExtensionData.o 

# inference rule
%.o: %.cpp
	@echo "  MPICXX" $<
	@$(MPICXX) -c -o $@ $<  $(CXXFLAGS)

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
	@echo HAVE_CLOCK_GETTIME = $(HAVE_CLOCK_GETTIME)
	@echo INTEL_COMPILER = $(INTEL_COMPILER)
	@echo VIRTUAL_SEQUENCER = $(VIRTUAL_SEQUENCER)
	@echo MPICXX = $(MPICXX)
	@echo GPROF = $(GPROF)
	@echo OPTIMIZE = $(OPTIMIZE)
	@echo ""
	@echo "Compilation and linking flags (generated automatically)"
	@echo ""
	@echo CXXFLAGS = $(CXXFLAGS)
	@echo LDFLAGS = $(LDFLAGS)
	@echo ""
	@touch showOptions
	
# how to make Ray
code/Ray: showOptions $(obj-y)
	@echo "  MPICXX $@"
	@$(MPICXX) $(LDFLAGS) $(obj-y) -o $@
	@echo $(PREFIX) > PREFIX
	@echo $(TARGETS) > TARGETS

readSimulator/VirtualNextGenSequencer: readSimulator/simulatePairedReads.cpp
	@$(CXX) -o $@ $< $(CXXFLAGS)
	@echo "  CXX $<"

clean:
	@rm -f $(TARGETS) $(obj-y) showOptions PREFIX TARGETS
	@echo CLEAN

manual:
	latex2html -split 0 -html_version 4.0,latin1,unicode InstructionManual.tex
	pdflatex InstructionManual.tex

install: 
	@scripts/install.sh
