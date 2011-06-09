# author: Sébastien Boisvert
# Makefile for the ray assembler
#
#############################################
# compilation options

# installation prefix with make install
PREFIX=install-prefix

# maximum k-mer length
# in nucleotides
# 32 nucleotides are stored on 64 bits
# 64 nucleotides are stored on 2x64 bits
# 96 nucleotides are stored on 3x64 bits
# Intermediate values should utilise intermediate numbers of bits.
MAXKMERLENGTH=64

# support for .gz files
# needs libz
# set to no if you don't have libz
# y/n
HAVE_LIBZ=y

# support for .bz2 files
# needs libbz2
# set to no if you don't have libbz2
# y/n
HAVE_LIBBZ2=y

# a real-time OS is available
# (for the function clock_gettime)
# y/n
HAVE_CLOCK_GETTIME=y

# use Intel's compiler
# the name of the Intel MPI C++ compiler is mpiicpc
# Open-MPI and MPICH2 utilise mpic++ for the name.
# y/n
INTEL_COMPILER=n

# pack structures to reduce memory usage
# will work on x86 and x86_64
# won't work on Itanium and on Sparc
# The operating system's kernel must also allow the retrieval of 8-byte chunks sitting
# between two memory cache pages. Linux can do that, but I don't know for others.
# if you are not sure, type uname -a to get your processor architecture.
# y/n
FORCE_PACKING=y

# compile assertions
# Ray may be faster when ASSERT=no
# y/n
ASSERT=y

# end of compilation options
#############################################

MPICXX-y = mpic++

# mpic++ from an MPI implementation must be reachable with the PATH
# tested implementations of MPI include Open-MPI and MPICH2
CXXFLAGS = -Wall -Icode

# optimization
CXXFLAGS += -O3 -fomit-frame-pointer

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

#compile with real-time linux
CXXFLAGS-$(HAVE_CLOCK_GETTIME) += -DHAVE_CLOCK_GETTIME 
LDFLAGS-$(HAVE_CLOCK_GETTIME) += -lrt

CXXFLAGS += $(CXXFLAGS-y)
LDFLAGS += $(LDFLAGS-y)

TARGET=code/Ray

#memory
obj-y += code/memory/OnDiskAllocator.o code/memory/ReusableMemoryStore.o code/memory/MyAllocator.o code/memory/RingAllocator.o \
code/memory/malloc_types.o 

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

#structures
obj-y += code/structures/VertexData.o code/structures/Kmer.o code/structures/MyForestIterator.o code/structures/MyForest.o \
code/structures/ArrayOfReads.o  code/structures/Direction.o code/structures/PairedRead.o code/structures/ReadAnnotation.o code/structures/Read.o  \
code/structures/StaticVector.o code/structures/Vertex.o

#scaffolder
obj-y += code/scaffolder/Scaffolder.o 

#unclassified
obj-y += code/VertexMessenger.o \
code/ReadFetcher.o code/LibraryWorker.o code/IndexerWorker.o  \
code/SeedWorker.o code/ExtensionElement.o \
code/DepthFirstSearchData.o code/MemoryConsumptionReducer.o  code/SeedingData.o \
code/BubbleTool.o code/Chooser.o code/EarlyStoppingTechnology.o code/FusionData.o code/Library.o code/Loader.o \
code/OpenAssemblerChooser.o code/SeedExtender.o code/SequencesIndexer.o code/SequencesLoader.o \
code/TimePrinter.o code/TipWatchdog.o code/VerticesExtractor.o  code/ray_main.o code/ExtensionData.o 

# inference rule
%.o: %.cpp
	$(MPICXX) -c -o $@ $<  $(CXXFLAGS)

# the target is Ray
all: $(TARGET)

# how to make Ray
$(TARGET): $(obj-y)
	$(MPICXX) $(LDFLAGS) $(obj-y) -o $@

clean:
	rm -f $(TARGET) $(obj-y)

manual:
	latex2html -split 0 -html_version 4.0,latin1,unicode InstructionManual.tex
	pdflatex InstructionManual.tex

install: all manual
	mkdir -p $(PREFIX)
	cp $(TARGET) $(PREFIX)
	cp InstructionManual.pdf $(PREFIX)
	cp README $(PREFIX)
	cp LICENSE $(PREFIX)
	cp ChangeLog $(PREFIX)

