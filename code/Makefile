

MPICXX = mpicxx

######## <Application>

# communication
obj-y += communication/NetworkTest.o
obj-y += communication/NetworkTest_adapters.o

obj-y += communication/MessageProcessor.o
obj-y += communication/MessageProcessor_adapters.o

# search engine
obj-y += search-engine/Searcher.o
obj-y += search-engine/Searcher_adapters.o

obj-y += search-engine/SearchDirectory.o
obj-y += search-engine/ContigSearchEntry.o
obj-y += search-engine/ContigHit.o
obj-y += search-engine/ColorSet.o
obj-y += search-engine/VirtualKmerColor.o

#formats
obj-y += format/ColorSpaceDecoder.o format/ColorSpaceLoader.o format/FastaLoader.o \
format/FastqLoader.o format/SffLoader.o \
format/Amos.o

#core

obj-y += core/Machine.o 
obj-y += core/MachineHelper.o
obj-y += core/Parameters.o core/common_functions.o

#compression

obj-$(HAVE_LIBBZ2) += compression/BzReader.o  

#cryptography
obj-y += cryptography/crypto.o
obj-$(HAVE_LIBBZ2) += format/FastqBz2Loader.o 
obj-$(HAVE_LIBZ) += format/FastqGzLoader.o 

#graph
obj-y += graph/GridTable.o graph/GridTableIterator.o graph/CoverageDistribution.o 
obj-y += graph/CoverageGatherer.o graph/KmerAcademy.o graph/KmerAcademyIterator.o

#structures

obj-y +=  structures/BloomFilter.o
obj-y += structures/Kmer.o \
structures/ArrayOfReads.o  structures/Direction.o \
 structures/PairedRead.o structures/ReadAnnotation.o structures/Read.o  \
structures/Vertex.o
obj-y += structures/AssemblySeed.o

#scaffolder
obj-y += scaffolder/Scaffolder.o 
obj-y += scaffolder/ScaffoldingLink.o
obj-y += scaffolder/SummarizedLink.o
obj-y += scaffolder/ScaffoldingAlgorithm.o
obj-y += scaffolder/ScaffoldingVertex.o
obj-y += scaffolder/ScaffoldingEdge.o

obj-y += pairs/LibraryPeakFinder.o

#assembler
obj-y += assembler/VertexMessenger.o \
assembler/ReadFetcher.o assembler/LibraryWorker.o assembler/IndexerWorker.o  \
assembler/SeedWorker.o assembler/ExtensionElement.o \
assembler/DepthFirstSearchData.o assembler/SeedingData.o \
assembler/FusionData.o assembler/Library.o assembler/Loader.o \
obj-y += assembler/Library_adapters.o

obj-y += assembler/SeedExtender.o assembler/SequencesIndexer.o \
assembler/SequencesLoader.o \
 assembler/VerticesExtractor.o \
assembler/ray_main.o assembler/ExtensionData.o 
obj-y += assembler/KmerAcademyBuilder.o
obj-y += assembler/EdgePurger.o assembler/EdgePurgerWorker.o
obj-y += assembler/Partitioner.o
obj-y += assembler/Partitioner_adapters.o

obj-y += assembler/FusionWorker.o 
obj-y += assembler/FusionTaskCreator.o
obj-y += assembler/JoinerWorker.o 
obj-y += assembler/JoinerTaskCreator.o

# heuristics
obj-y += heuristics/BubbleTool.o heuristics/Chooser.o heuristics/OpenAssemblerChooser.o \
 heuristics/TipWatchdog.o heuristics/NovaEngine.o


######## </Application>

#assume that we want to use Ray Platform shipped with Ray Application
ifndef $(RAY_PLATFORM)
	RAY_PLATFORM=../RayPlatform
endif

ifndef $(RAY_APPLICATION_SCRIPTS)
	RAY_APPLICATION_SCRIPTS=./scripting
endif

ifndef $(RAY_APPLICATION)
	RAY_APPLICATION=.
endif

all: libRayApplication.a

libRayApplication.a: $(obj-y)
	$(AR) rcs  libRayApplication.a $(obj-y)

# inference rule
%.o: %.cpp
	$(MPICXX) -c -o $@ $<  $(CXXFLAGS) -I$(RAY_PLATFORM) -I$(RAY_APPLICATION) -I$(RAY_APPLICATION_SCRIPTS)

clean:
	@rm -f libRayApplication.a $(obj-y)
	@echo CLEAN

