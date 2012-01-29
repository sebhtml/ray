cmake_minimum_required(VERSION 2.6 )

set( CMAKE_CXX_COMPILER  mpicxx )
set( CMAKE_CXX_FLAGS "-O3 -Wall -ansi" )

include_directories( . ../RayPlatform ./scripting )

set( core 
 core/Machine.cpp 
 core/Parameters.cpp core/common_functions.cpp 
)

set( plugin_VerticesExtractor 
assembler/VerticesExtractor_adapters.cpp
 assembler/VerticesExtractor.cpp 
 graph/GridTable.cpp graph/GridTableIterator.cpp
 structures/Vertex.cpp )

set( plugin_EdgePurger  
assembler/EdgePurger_adapters.cpp
 assembler/EdgePurger.cpp assembler/EdgePurgerWorker.cpp )

set( plugin_KmerAcademyBuilder 
assembler/KmerAcademyBuilder_adapters.cpp
 assembler/KmerAcademyBuilder.cpp
  structures/BloomFilter.cpp
 structures/Kmer.cpp
 graph/KmerAcademy.cpp graph/KmerAcademyIterator.cpp )

set( plugin_SequencesLoader
 assembler/SequencesLoader_adapters.cpp 
 assembler/SequencesLoader.cpp 
 structures/Read.cpp  structures/ArrayOfReads.cpp 
 format/ColorSpaceDecoder.cpp format/ColorSpaceLoader.cpp 
 format/FastaLoader.cpp format/FastqLoader.cpp format/SffLoader.cpp  assembler/Loader.cpp  
 compression/BzReader.cpp  
 format/FastqBz2Loader.cpp 
 format/FastqGzLoader.cpp  )

set( plugin_FusionTaskCreator
 assembler/FusionTaskCreator_adapters.cpp
 assembler/FusionWorker.cpp 
 assembler/FusionTaskCreator.cpp )

set( plugin_FusionData
 assembler/FusionData_adapters.cpp
 assembler/FusionData.cpp  )

set( plugin_Scaffolder
 scaffolder/Scaffolder_adapters.cpp
 scaffolder/Scaffolder.cpp 
 scaffolder/ScaffoldingLink.cpp
 scaffolder/SummarizedLink.cpp
 scaffolder/ScaffoldingAlgorithm.cpp
 scaffolder/ScaffoldingVertex.cpp
 scaffolder/ScaffoldingEdge.cpp )

set( plugin_Searcher
search-engine/Searcher_adapters.cpp
search-engine/Searcher.cpp
search-engine/SearchDirectory.cpp
search-engine/ContigSearchEntry.cpp
search-engine/ContigHit.cpp
search-engine/ColorSet.cpp
search-engine/VirtualKmerColor.cpp )

set( plugin_CoverageGatherer
graph/CoverageGatherer.cpp 
graph/CoverageGatherer_adapters.cpp
graph/CoverageDistribution.cpp  )

set( plugin_Partitioner
 assembler/Partitioner.cpp
 assembler/Partitioner_adapters.cpp )

set(plugin_MessageProcessor
communication/MessageProcessor_adapters.cpp
communication/MessageProcessor.cpp )

set( plugin_SequencesIndexer
assembler/SequencesIndexer_adapters.cpp 
assembler/SequencesIndexer.cpp 
assembler/IndexerWorker.cpp 
 structures/PairedRead.cpp structures/ReadAnnotation.cpp )

set(plugin_Amos
format/Amos_adapters.cpp
format/Amos.cpp )

set(plugin_NetworkTest
 communication/NetworkTest_adapters.cpp
 communication/NetworkTest.cpp )

set(plugin_JoinerTaskCreator
assembler/JoinerTaskCreator_adapters.cpp
assembler/JoinerWorker.cpp 
assembler/JoinerTaskCreator.cpp )

set(plugin_MachineHelper
core/MachineHelper_adapters.cpp
core/MachineHelper.cpp )

set(plugin_SeedingData
structures/AssemblySeed.cpp assembler/SeedWorker.cpp
assembler/SeedingData_adapters.cpp
assembler/SeedingData.cpp )

set(plugin_Library
assembler/Library_adapters.cpp
pairs/LibraryPeakFinder.cpp assembler/LibraryWorker.cpp 
assembler/Library.cpp  )

set(plugin_SeedExtender
assembler/SeedExtender_adapters.cpp
assembler/SeedExtender.cpp
 structures/Direction.cpp
assembler/VertexMessenger.cpp assembler/ReadFetcher.cpp
heuristics/BubbleTool.cpp heuristics/Chooser.cpp heuristics/OpenAssemblerChooser.cpp
heuristics/TipWatchdog.cpp heuristics/NovaEngine.cpp
assembler/ExtensionElement.cpp 
assembler/DepthFirstSearchData.cpp 
assembler/ExtensionData.cpp )

set(ray_source 
assembler/ray_main.cpp 
)

# build static Ray

add_library( core_STATIC STATIC ${core})
set_target_properties( core_STATIC PROPERTIES PREFIX "")
set_target_properties( core_STATIC PROPERTIES OUTPUT_NAME ray_core )
add_library( plugin_VerticesExtractor_STATIC STATIC ${plugin_VerticesExtractor} )
set_target_properties( plugin_VerticesExtractor_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_VerticesExtractor_STATIC PROPERTIES OUTPUT_NAME ray_plugin_VerticesExtractor )
add_library( plugin_EdgePurger_STATIC STATIC ${plugin_EdgePurger} )
set_target_properties( plugin_EdgePurger_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_EdgePurger_STATIC PROPERTIES OUTPUT_NAME ray_plugin_EdgePurger )
add_library( plugin_KmerAcademyBuilder_STATIC STATIC ${plugin_KmerAcademyBuilder} )
set_target_properties( plugin_KmerAcademyBuilder_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_KmerAcademyBuilder_STATIC PROPERTIES OUTPUT_NAME ray_plugin_KmerAcademyBuilder )
add_library( plugin_SequencesLoader_STATIC STATIC ${plugin_SequencesLoader} )
set_target_properties( plugin_SequencesLoader_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_SequencesLoader_STATIC PROPERTIES OUTPUT_NAME ray_plugin_SequencesLoader )
add_library( plugin_FusionTaskCreator_STATIC STATIC ${plugin_FusionTaskCreator} )
set_target_properties( plugin_FusionTaskCreator_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_FusionTaskCreator_STATIC PROPERTIES OUTPUT_NAME ray_plugin_FusionTaskCreator )
add_library( plugin_FusionData_STATIC STATIC ${plugin_FusionData} )
set_target_properties( plugin_FusionData_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_FusionData_STATIC PROPERTIES OUTPUT_NAME ray_plugin_FusionData )
add_library( plugin_Scaffolder_STATIC STATIC ${plugin_Scaffolder} )
set_target_properties( plugin_Scaffolder_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_Scaffolder_STATIC PROPERTIES OUTPUT_NAME ray_plugin_Scaffolder )
add_library( plugin_Searcher_STATIC STATIC ${plugin_Searcher} )
set_target_properties( plugin_Searcher_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_Searcher_STATIC PROPERTIES OUTPUT_NAME ray_plugin_Searcher )
add_library( plugin_CoverageGatherer_STATIC STATIC ${plugin_CoverageGatherer} )
set_target_properties( plugin_CoverageGatherer_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_CoverageGatherer_STATIC PROPERTIES OUTPUT_NAME ray_plugin_CoverageGatherer )
add_library( plugin_Partitioner_STATIC STATIC ${plugin_Partitioner} )
set_target_properties( plugin_Partitioner_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_Partitioner_STATIC PROPERTIES OUTPUT_NAME ray_plugin_Partitioner )
add_library( plugin_MessageProcessor_STATIC STATIC ${plugin_MessageProcessor} )
set_target_properties( plugin_MessageProcessor_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_MessageProcessor_STATIC PROPERTIES OUTPUT_NAME ray_plugin_MessageProcessor )
add_library( plugin_SequencesIndexer_STATIC STATIC ${plugin_SequencesIndexer} )
set_target_properties( plugin_SequencesIndexer_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_SequencesIndexer_STATIC PROPERTIES OUTPUT_NAME ray_plugin_SequencesIndexer )
add_library( plugin_Amos_STATIC STATIC ${plugin_Amos} )
set_target_properties( plugin_Amos_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_Amos_STATIC PROPERTIES OUTPUT_NAME ray_plugin_Amos )
add_library( plugin_NetworkTest_STATIC STATIC ${plugin_NetworkTest} )
set_target_properties( plugin_NetworkTest_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_NetworkTest_STATIC PROPERTIES OUTPUT_NAME ray_plugin_NetworkTest )
add_library( plugin_JoinerTaskCreator_STATIC STATIC ${plugin_JoinerTaskCreator} )
set_target_properties( plugin_JoinerTaskCreator_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_JoinerTaskCreator_STATIC PROPERTIES OUTPUT_NAME ray_plugin_JoinerTaskCreator )
add_library( plugin_MachineHelper_STATIC STATIC ${plugin_MachineHelper} )
set_target_properties( plugin_MachineHelper_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_MachineHelper_STATIC PROPERTIES OUTPUT_NAME ray_plugin_MachineHelper )
add_library( plugin_SeedingData_STATIC STATIC ${plugin_SeedingData} )
set_target_properties( plugin_SeedingData_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_SeedingData_STATIC PROPERTIES OUTPUT_NAME ray_plugin_SeedingData )
add_library( plugin_Library_STATIC STATIC ${plugin_Library} )
set_target_properties( plugin_Library_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_Library_STATIC PROPERTIES OUTPUT_NAME ray_plugin_Library )
add_library( plugin_SeedExtender_STATIC STATIC ${plugin_SeedExtender} )
set_target_properties( plugin_SeedExtender_STATIC PROPERTIES PREFIX "")
set_target_properties( plugin_SeedExtender_STATIC PROPERTIES OUTPUT_NAME ray_plugin_SeedExtender )

add_executable(Ray ${ray_source})

find_library( RayPlatform_STATIC NAMES libRayPlatform.a PATHS RayPlatform )

target_link_libraries(Ray
RayPlatform_STATIC
core_STATIC
plugin_VerticesExtractor_STATIC
plugin_EdgePurger_STATIC
plugin_KmerAcademyBuilder_STATIC
plugin_SequencesLoader_STATIC
plugin_FusionTaskCreator_STATIC
plugin_FusionData_STATIC
plugin_Scaffolder_STATIC
plugin_Searcher_STATIC
plugin_CoverageGatherer_STATIC
plugin_Partitioner_STATIC
plugin_MessageProcessor_STATIC
plugin_SequencesIndexer_STATIC
plugin_Amos_STATIC
plugin_NetworkTest_STATIC
plugin_JoinerTaskCreator_STATIC
plugin_MachineHelper_STATIC
plugin_SeedingData_STATIC
plugin_Library_STATIC
plugin_SeedExtender_STATIC 
)





# build shared Ray

add_library( core_SHARED SHARED ${core})
set_target_properties( core_SHARED PROPERTIES PREFIX "")
set_target_properties( core_SHARED PROPERTIES OUTPUT_NAME ray_core )
add_library( plugin_VerticesExtractor_SHARED SHARED ${plugin_VerticesExtractor} )
set_target_properties( plugin_VerticesExtractor_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_VerticesExtractor_SHARED PROPERTIES OUTPUT_NAME ray_plugin_VerticesExtractor )
add_library( plugin_EdgePurger_SHARED SHARED ${plugin_EdgePurger} )
set_target_properties( plugin_EdgePurger_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_EdgePurger_SHARED PROPERTIES OUTPUT_NAME ray_plugin_EdgePurger )
add_library( plugin_KmerAcademyBuilder_SHARED SHARED ${plugin_KmerAcademyBuilder} )
set_target_properties( plugin_KmerAcademyBuilder_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_KmerAcademyBuilder_SHARED PROPERTIES OUTPUT_NAME ray_plugin_KmerAcademyBuilder )
add_library( plugin_SequencesLoader_SHARED SHARED ${plugin_SequencesLoader} )
set_target_properties( plugin_SequencesLoader_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_SequencesLoader_SHARED PROPERTIES OUTPUT_NAME ray_plugin_SequencesLoader )
add_library( plugin_FusionTaskCreator_SHARED SHARED ${plugin_FusionTaskCreator} )
set_target_properties( plugin_FusionTaskCreator_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_FusionTaskCreator_SHARED PROPERTIES OUTPUT_NAME ray_plugin_FusionTaskCreator )
add_library( plugin_FusionData_SHARED SHARED ${plugin_FusionData} )
set_target_properties( plugin_FusionData_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_FusionData_SHARED PROPERTIES OUTPUT_NAME ray_plugin_FusionData )
add_library( plugin_Scaffolder_SHARED SHARED ${plugin_Scaffolder} )
set_target_properties( plugin_Scaffolder_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_Scaffolder_SHARED PROPERTIES OUTPUT_NAME ray_plugin_Scaffolder )
add_library( plugin_Searcher_SHARED SHARED ${plugin_Searcher} )
set_target_properties( plugin_Searcher_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_Searcher_SHARED PROPERTIES OUTPUT_NAME ray_plugin_Searcher )
add_library( plugin_CoverageGatherer_SHARED SHARED ${plugin_CoverageGatherer} )
set_target_properties( plugin_CoverageGatherer_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_CoverageGatherer_SHARED PROPERTIES OUTPUT_NAME ray_plugin_CoverageGatherer )
add_library( plugin_Partitioner_SHARED SHARED ${plugin_Partitioner} )
set_target_properties( plugin_Partitioner_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_Partitioner_SHARED PROPERTIES OUTPUT_NAME ray_plugin_Partitioner )
add_library( plugin_MessageProcessor_SHARED SHARED ${plugin_MessageProcessor} )
set_target_properties( plugin_MessageProcessor_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_MessageProcessor_SHARED PROPERTIES OUTPUT_NAME ray_plugin_MessageProcessor )
add_library( plugin_SequencesIndexer_SHARED SHARED ${plugin_SequencesIndexer} )
set_target_properties( plugin_SequencesIndexer_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_SequencesIndexer_SHARED PROPERTIES OUTPUT_NAME ray_plugin_SequencesIndexer )
add_library( plugin_Amos_SHARED SHARED ${plugin_Amos} )
set_target_properties( plugin_Amos_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_Amos_SHARED PROPERTIES OUTPUT_NAME ray_plugin_Amos )
add_library( plugin_NetworkTest_SHARED SHARED ${plugin_NetworkTest} )
set_target_properties( plugin_NetworkTest_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_NetworkTest_SHARED PROPERTIES OUTPUT_NAME ray_plugin_NetworkTest )
add_library( plugin_JoinerTaskCreator_SHARED SHARED ${plugin_JoinerTaskCreator} )
set_target_properties( plugin_JoinerTaskCreator_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_JoinerTaskCreator_SHARED PROPERTIES OUTPUT_NAME ray_plugin_JoinerTaskCreator )
add_library( plugin_MachineHelper_SHARED SHARED ${plugin_MachineHelper} )
set_target_properties( plugin_MachineHelper_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_MachineHelper_SHARED PROPERTIES OUTPUT_NAME ray_plugin_MachineHelper )
add_library( plugin_SeedingData_SHARED SHARED ${plugin_SeedingData} )
set_target_properties( plugin_SeedingData_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_SeedingData_SHARED PROPERTIES OUTPUT_NAME ray_plugin_SeedingData )
add_library( plugin_Library_SHARED SHARED ${plugin_Library} )
set_target_properties( plugin_Library_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_Library_SHARED PROPERTIES OUTPUT_NAME ray_plugin_Library )
add_library( plugin_SeedExtender_SHARED SHARED ${plugin_SeedExtender} )
set_target_properties( plugin_SeedExtender_SHARED PROPERTIES PREFIX "")
set_target_properties( plugin_SeedExtender_SHARED PROPERTIES OUTPUT_NAME ray_plugin_SeedExtender )

add_executable(Ray_shared ${ray_source})

find_library( RayPlatform_SHARED NAMES libRayPlatform.so PATHS RayPlatform )

target_link_libraries(Ray_shared 
RayPlatform_SHARED
core_SHARED
plugin_VerticesExtractor_SHARED
plugin_EdgePurger_SHARED
plugin_KmerAcademyBuilder_SHARED
plugin_SequencesLoader_SHARED
plugin_FusionTaskCreator_SHARED
plugin_FusionData_SHARED
plugin_Scaffolder_SHARED
plugin_Searcher_SHARED
plugin_CoverageGatherer_SHARED
plugin_Partitioner_SHARED
plugin_MessageProcessor_SHARED
plugin_SequencesIndexer_SHARED
plugin_Amos_SHARED
plugin_NetworkTest_SHARED
plugin_JoinerTaskCreator_SHARED
plugin_MachineHelper_SHARED
plugin_SeedingData_SHARED
plugin_Library_SHARED
plugin_SeedExtender_SHARED 
)



