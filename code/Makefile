

# core 

obj-y += assembler/ray_main.o 
obj-y += core/Machine.o 
obj-y += core/Parameters.o core/common_functions.o

obj-y += graph/GridTable.o graph/GridTableIterator.o
obj-y += structures/Vertex.o
obj-y += assembler/VerticesExtractor.o 

obj-y += assembler/EdgePurger.o assembler/EdgePurgerWorker.o

# plugin KmerAcademyBuilder

obj-y += assembler/KmerAcademyBuilder_adapters.o
obj-y += assembler/KmerAcademyBuilder.o
obj-y +=  structures/BloomFilter.o
obj-y += structures/Kmer.o
obj-y += graph/KmerAcademy.o graph/KmerAcademyIterator.o

# plugin SequencesLoader

obj-y += assembler/SequencesLoader_adapters.o 
obj-y += assembler/SequencesLoader.o 
obj-y += structures/Read.o  structures/ArrayOfReads.o 
obj-y += format/ColorSpaceDecoder.o format/ColorSpaceLoader.o 
obj-y += format/FastaLoader.o format/FastqLoader.o format/SffLoader.o  assembler/Loader.o 
obj-$(HAVE_LIBBZ2) += compression/BzReader.o  
obj-$(HAVE_LIBBZ2) += format/FastqBz2Loader.o 
obj-$(HAVE_LIBZ) += format/FastqGzLoader.o 

# plugin FusionTaskCreator

obj-y += assembler/FusionTaskCreator_adapters.o
obj-y += assembler/FusionWorker.o 
obj-y += assembler/FusionTaskCreator.o 

# plugin FusionData

obj-y += assembler/FusionData_adapters.o
obj-y += assembler/FusionData.o 

# plugin Scaffolder

obj-y += scaffolder/Scaffolder_adapters.o
obj-y += scaffolder/Scaffolder.o 
obj-y += scaffolder/ScaffoldingLink.o
obj-y += scaffolder/SummarizedLink.o
obj-y += scaffolder/ScaffoldingAlgorithm.o
obj-y += scaffolder/ScaffoldingVertex.o
obj-y += scaffolder/ScaffoldingEdge.o

# plugin Searcher

obj-y += search-engine/Searcher_adapters.o
obj-y += search-engine/Searcher.o
obj-y += search-engine/SearchDirectory.o
obj-y += search-engine/ContigSearchEntry.o
obj-y += search-engine/ContigHit.o
obj-y += search-engine/ColorSet.o
obj-y += search-engine/VirtualKmerColor.o

# plugin CoverageGatherer

obj-y += graph/CoverageGatherer.o 
obj-y += graph/CoverageGatherer_adapters.o
obj-y += graph/CoverageDistribution.o 

# plugin Partitioner

obj-y += assembler/Partitioner.o
obj-y += assembler/Partitioner_adapters.o

# plugin MessageProcessor

obj-y += communication/MessageProcessor_adapters.o
obj-y += communication/MessageProcessor.o

# plugin SequencesIndexer

obj-y += assembler/SequencesIndexer_adapters.o 
obj-y += assembler/SequencesIndexer.o 
obj-y += assembler/IndexerWorker.o 
obj-y +=  structures/PairedRead.o structures/ReadAnnotation.o 

# plugin Amos

obj-y += format/Amos_adapters.o
obj-y += format/Amos.o

# plugin NetworkTest

obj-y += communication/NetworkTest_adapters.o
obj-y += communication/NetworkTest.o

# plugin JoinerTaskCreator

obj-y += assembler/JoinerTaskCreator_adapters.o
obj-y += assembler/JoinerWorker.o 
obj-y += assembler/JoinerTaskCreator.o

# plugin MachineHelper

obj-y += core/MachineHelper_adapters.o
obj-y += core/MachineHelper.o

# plugin SeedingData

obj-y += structures/AssemblySeed.o assembler/SeedWorker.o
obj-y += assembler/SeedingData_adapters.o
obj-y += assembler/SeedingData.o 

# plugin Library

obj-y += assembler/Library_adapters.o
obj-y += pairs/LibraryPeakFinder.o assembler/LibraryWorker.o 
obj-y += assembler/Library.o 

# plugin SeedExtender

obj-y += assembler/SeedExtender_adapters.o
obj-y += assembler/SeedExtender.o
obj-y +=  structures/Direction.o
obj-y += assembler/VertexMessenger.o assembler/ReadFetcher.o
obj-y += heuristics/BubbleTool.o heuristics/Chooser.o heuristics/OpenAssemblerChooser.o
obj-y += heuristics/TipWatchdog.o heuristics/NovaEngine.o
obj-y += assembler/ExtensionElement.o 
obj-y += assembler/DepthFirstSearchData.o 
obj-y += assembler/ExtensionData.o 


#######################################################

MPICXX = mpicxx
AR= ar

#assume that we want to use Ray Platform shipped with Ray Application
ifndef $(RAY_PLATFORM)
	RAY_PLATFORM=../RayPlatform
endif

all: TheRayGenomeAssembler.a

TheRayGenomeAssembler.a: $(obj-y)
	$(AR) rcs  TheRayGenomeAssembler.a $(obj-y)

# inference rule
%.o: %.cpp
	$(MPICXX) -c -o $@ $<  $(CXXFLAGS) -I$(RAY_PLATFORM) -I. -I./scripting

clean:
	@rm -f TheRayGenomeAssembler.a $(obj-y)
	@echo CLEAN

