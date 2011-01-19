# primary Makefile.am
# Author: Sébastien Boisvert
# Centre de recherche en infectiologie, Université Laval
# Copyright 2010, or so


AM_CXXFLAGS=-O3 -Wall
LDADD=""

noinst_LIBRARIES = libRay.a
libRay_a_SOURCES = \
VirtualCommunicator.h VirtualCommunicator.cpp \
MemoryConsumptionReducer.h MemoryConsumptionReducer.cpp \
MyForestIterator.cpp MyForestIterator.h crypto.cpp crypto.h \
MyStack.h EarlyStoppingTechnology.h EarlyStoppingTechnology.cpp ArrayOfReads.h ArrayOfReads.cpp \
RingAllocator.cpp RingAllocator.h StaticVector.cpp StaticVector.h \
MessagesHandler.h MessagesHandler.cpp slave_modes.h master_modes.h mpi_tags.h \
SeedingData.cpp SeedingData.h Library.h Library.cpp FastaLoader.h FastaLoader.cpp FastqLoader.h \
FastqLoader.cpp BubbleData.h BubbleTool.cpp BubbleTool.h BufferedData.cpp \
BufferedData.h Chooser.cpp ChooserData.h Chooser.h ColorSpaceDecoder.cpp \
ColorSpaceDecoder.h ColorSpaceLoader.cpp ColorSpaceLoader.h common_functions.cpp \
common_functions.h config.h CoverageDistribution.cpp CoverageDistribution.h \
DepthFirstSearchData.cpp DepthFirstSearchData.h Direction.cpp Direction.h \
  ExtensionData.cpp ExtensionData.h \
FusionData.h FusionData.cpp Loader.cpp Loader.hi Machine.cpp Machine.h Message.cpp Message.h \
MessageProcessor.cpp MessageProcessor.h MyAllocator.cpp MyAllocator.h MyForest.cpp \
MyForest.h OpenAssemblerChooser.cpp OpenAssemblerChooser.h PairedRead.cpp \
PairedRead.h Parameters.cpp Parameters.h ReadAnnotation.cpp ReadAnnotation.h \
Read.cpp Read.h SeedExtender.cpp \
SeedExtender.h SequencesIndexer.cpp SequencesIndexer.h SequencesLoader.cpp SequencesLoader.h \
SffLoader.cpp SffLoader.h SplayNode.h SplayTree.h SplayTreeIterator.h TimePrinter.cpp \
TimePrinter.h TipWatchdog.cpp TipWatchdog.h Vertex.cpp Vertex.h VertexLinkedList.cpp \
VertexLinkedList.h VerticesExtractor.cpp VerticesExtractor.h 

if HAVE_ZLIB
libRay_a_SOURCES += FastqGzLoader.cpp FastqGzLoader.h
endif

if HAVE_BZ2
libRay_a_SOURCES += FastqBz2Loader.cpp FastqBz2Loader.h BzReader.h BzReader.cpp
endif

bin_PROGRAMS = Ray

Ray_SOURCES =  ray_main.cpp 
Ray_LDADD = libRay.a

if HAVE_ZLIB
Ray_LDADD += -lz
endif

if HAVE_BZ2
Ray_LDADD += -lbz2
endif
