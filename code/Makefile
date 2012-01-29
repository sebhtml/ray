# author Sébastien Boisvert
#
# based on http://www.ravnborg.org/kbuild/makefiles.html
#
# the code is distributed in a small Ray core that is built on top
# of the Ray platform.
#
# Then, plugins (interface CorePlugin in the Ray Platform)
# are simply added onto the core (class ComputeCore in the
# Ray Platform).

MPICXX = mpicxx
LD = ld
RM = rm
CXXFLAGS= -O3 -Wall -ansi

# core 

core-y += assembler/ray_main.o core/Machine.o 
core-y += core/Parameters.o core/common_functions.o

ray_core.o: $(core-y)
	$(LD) -r $^ -o $@

obj-y += $(core-y)

# plugin VerticesExtractor

VerticesExtractor-y += assembler/VerticesExtractor_adapters.o
VerticesExtractor-y += assembler/VerticesExtractor.o 
VerticesExtractor-y += graph/GridTable.o graph/GridTableIterator.o
VerticesExtractor-y += structures/Vertex.o

ray_plugin_VerticesExtractor.o: $(VerticesExtractor-y)
	$(LD) -r $^ -o $@

obj-y += $(VerticesExtractor-y)

PLUGINS-y += ray_plugin_VerticesExtractor.o

# plugin EdgePurger

EdgePurger-y += assembler/EdgePurger_adapters.o
EdgePurger-y += assembler/EdgePurger.o assembler/EdgePurgerWorker.o

ray_plugin_EdgePurger.o: $(EdgePurger-y)
	$(LD) -r $^ -o $@

obj-y += $(EdgePurger-y)

PLUGINS-y += ray_plugin_EdgePurger.o

# plugin KmerAcademyBuilder

KmerAcademyBuilder-y += assembler/KmerAcademyBuilder_adapters.o
KmerAcademyBuilder-y += assembler/KmerAcademyBuilder.o
KmerAcademyBuilder-y +=  structures/BloomFilter.o
KmerAcademyBuilder-y += structures/Kmer.o
KmerAcademyBuilder-y += graph/KmerAcademy.o graph/KmerAcademyIterator.o

ray_plugin_KmerAcademyBuilder.o: $(KmerAcademyBuilder-y)
	$(LD) -r $^ -o $@

obj-y += $(KmerAcademyBuilder-y)

PLUGINS-y += ray_plugin_KmerAcademyBuilder.o

# plugin SequencesLoader

SequencesLoader-y += assembler/SequencesLoader_adapters.o 
SequencesLoader-y += assembler/SequencesLoader.o 
SequencesLoader-y += structures/Read.o  structures/ArrayOfReads.o 
SequencesLoader-y += format/ColorSpaceDecoder.o format/ColorSpaceLoader.o 
SequencesLoader-y += format/FastaLoader.o format/FastqLoader.o format/SffLoader.o  assembler/Loader.o 
SequencesLoader-$(HAVE_LIBBZ2) += compression/BzReader.o  
SequencesLoader-$(HAVE_LIBBZ2) += format/FastqBz2Loader.o 
SequencesLoader-$(HAVE_LIBZ) += format/FastqGzLoader.o 

ray_plugin_SequencesLoader.o: $(SequencesLoader-y)
	$(LD) -r $^ -o $@

obj-y += $(KmerAcademyBuilder-y)

PLUGINS-y += ray_plugin_SequencesLoader.o

# plugin FusionTaskCreator

FusionTaskCreator-y += assembler/FusionTaskCreator_adapters.o
FusionTaskCreator-y += assembler/FusionWorker.o 
FusionTaskCreator-y += assembler/FusionTaskCreator.o 

ray_plugin_FusionTaskCreator.o: $(FusionTaskCreator-y)
	$(LD) -r $^ -o $@

obj-y += $(FusionTaskCreator-y)

PLUGINS-y += ray_plugin_FusionTaskCreator.o

# plugin FusionData

FusionData-y += assembler/FusionData_adapters.o
FusionData-y += assembler/FusionData.o 

ray_plugin_FusionData.o: $(FusionData-y)
	$(LD) -r $^ -o $@

obj-y += $(FusionData-y)

PLUGINS-y += ray_plugin_FusionData.o

# plugin Scaffolder

Scaffolder-y += scaffolder/Scaffolder_adapters.o
Scaffolder-y += scaffolder/Scaffolder.o 
Scaffolder-y += scaffolder/ScaffoldingLink.o
Scaffolder-y += scaffolder/SummarizedLink.o
Scaffolder-y += scaffolder/ScaffoldingAlgorithm.o
Scaffolder-y += scaffolder/ScaffoldingVertex.o
Scaffolder-y += scaffolder/ScaffoldingEdge.o

ray_plugin_Scaffolder.o: $(Scaffolder-y)
	$(LD) -r $^ -o $@

obj-y += $(Scaffolder-y)

PLUGINS-y += ray_plugin_Scaffolder.o

# plugin Searcher

Searcher-y += search-engine/Searcher_adapters.o
Searcher-y += search-engine/Searcher.o
Searcher-y += search-engine/SearchDirectory.o
Searcher-y += search-engine/ContigSearchEntry.o
Searcher-y += search-engine/ContigHit.o
Searcher-y += search-engine/ColorSet.o
Searcher-y += search-engine/VirtualKmerColor.o

ray_plugin_Searcher.o: $(Searcher-y)
	$(LD) -r $^ -o $@

obj-y += $(Searcher-y)

PLUGINS-y += ray_plugin_Searcher.o

# plugin CoverageGatherer

CoverageGatherer-y += graph/CoverageGatherer.o 
CoverageGatherer-y += graph/CoverageGatherer_adapters.o
CoverageGatherer-y += graph/CoverageDistribution.o 

ray_plugin_CoverageGatherer.o: $(CoverageGatherer-y)
	$(LD) -r $^ -o $@

obj-y += $(CoverageGatherer-y)

PLUGINS-y += ray_plugin_CoverageGatherer.o

# plugin Partitioner

Partitioner-y += assembler/Partitioner.o
Partitioner-y += assembler/Partitioner_adapters.o

ray_plugin_Partitioner.o: $(Partitioner-y)
	$(LD) -r $^ -o $@

obj-y += $(Partitioner-y)

PLUGINS-y += ray_plugin_Partitioner.o

# plugin MessageProcessor

MessageProcessor-y += communication/MessageProcessor_adapters.o
MessageProcessor-y += communication/MessageProcessor.o

ray_plugin_MessageProcessor.o: $(MessageProcessor-y)
	$(LD) -r $^ -o $@

obj-y += $(MessageProcessor-y)

PLUGINS-y += ray_plugin_MessageProcessor.o

# plugin SequencesIndexer

SequencesIndexer-y += assembler/SequencesIndexer_adapters.o 
SequencesIndexer-y += assembler/SequencesIndexer.o 
SequencesIndexer-y += assembler/IndexerWorker.o 
SequencesIndexer-y +=  structures/PairedRead.o structures/ReadAnnotation.o 

ray_plugin_SequencesIndexer.o: $(SequencesIndexer-y)
	$(LD) -r $^ -o $@

obj-y += $(SequencesIndexer-y)

PLUGINS-y += ray_plugin_SequencesIndexer.o

# plugin Amos

Amos-y += format/Amos_adapters.o
Amos-y += format/Amos.o

ray_plugin_Amos.o: $(Amos-y)
	$(LD) -r $^ -o $@

obj-y += $(Amos-y)

PLUGINS-y += ray_plugin_Amos.o

# plugin NetworkTest

NetworkTest-y += communication/NetworkTest_adapters.o
NetworkTest-y += communication/NetworkTest.o

ray_plugin_NetworkTest.o: $(NetworkTest-y)
	$(LD) -r $^ -o $@

obj-y += $(NetworkTest-y)

PLUGINS-y += ray_plugin_NetworkTest.o

# plugin JoinerTaskCreator

JoinerTaskCreator-y += assembler/JoinerTaskCreator_adapters.o
JoinerTaskCreator-y += assembler/JoinerWorker.o 
JoinerTaskCreator-y += assembler/JoinerTaskCreator.o

ray_plugin_JoinerTaskCreator.o: $(JoinerTaskCreator-y)
	$(LD) -r $^ -o $@

obj-y += $(JoinerTaskCreator-y)

PLUGINS-y += ray_plugin_JoinerTaskCreator.o

# plugin MachineHelper

MachineHelper-y += core/MachineHelper_adapters.o
MachineHelper-y += core/MachineHelper.o

ray_plugin_MachineHelper.o: $(MachineHelper-y)
	$(LD) -r $^ -o $@

obj-y += $(MachineHelper-y)

PLUGINS-y += ray_plugin_MachineHelper.o

# plugin SeedingData

SeedingData-y += structures/AssemblySeed.o assembler/SeedWorker.o
SeedingData-y += assembler/SeedingData_adapters.o
SeedingData-y += assembler/SeedingData.o 

ray_plugin_SeedingData.o: $(SeedingData-y)
	$(LD) -r $^ -o $@

obj-y += $(SeedingData-y)

PLUGINS-y += ray_plugin_SeedingData.o

# plugin Library

Library-y += assembler/Library_adapters.o
Library-y += pairs/LibraryPeakFinder.o assembler/LibraryWorker.o 
Library-y += assembler/Library.o 

ray_plugin_Library.o: $(Library-y)
	$(LD) -r $^ -o $@

obj-y += $(Library-y)

PLUGINS-y += ray_plugin_Library.o

# plugin SeedExtender

SeedExtender-y += assembler/SeedExtender_adapters.o
SeedExtender-y += assembler/SeedExtender.o
SeedExtender-y +=  structures/Direction.o
SeedExtender-y += assembler/VertexMessenger.o assembler/ReadFetcher.o
SeedExtender-y += heuristics/BubbleTool.o heuristics/Chooser.o heuristics/OpenAssemblerChooser.o
SeedExtender-y += heuristics/TipWatchdog.o heuristics/NovaEngine.o
SeedExtender-y += assembler/ExtensionElement.o 
SeedExtender-y += assembler/DepthFirstSearchData.o 
SeedExtender-y += assembler/ExtensionData.o 

ray_plugin_SeedExtender.o: $(SeedExtender-y)
	$(LD) -r $^ -o $@

obj-y += $(SeedExtender-y)

PLUGINS-y += ray_plugin_SeedExtender.o

#assume that we want to use Ray Platform shipped with Ray Application
ifndef $(RAY_PLATFORM)
	RAY_PLATFORM=../RayPlatform
endif

all: ray_core.o $(PLUGINS-y)

# inference rule
%.o: %.cpp
	$(MPICXX) $(CXXFLAGS) -I$(RAY_PLATFORM) -I. -I./scripting -c -o $@ $<

clean:
	$(RM) -f ray_core.o $(PLUGINS-y) $(obj-y)


