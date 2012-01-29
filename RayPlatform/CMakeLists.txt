cmake_minimum_required(VERSION 2.6 )

set( CMAKE_CXX_COMPILER  mpicxx )
set( CMAKE_CXX_FLAGS "-O3 -Wall -ansi" )

include_directories( . )

set( source_files

handlers/MessageTagHandler.cpp
handlers/SlaveModeHandler.cpp
handlers/MasterModeHandler.cpp
core/master_modes.cpp
core/ComputeCore.cpp
core/slave_modes.cpp
core/OperatingSystem.cpp
core/statistics.cpp
routing/GraphImplementationExperimental.cpp
routing/GraphImplementationKautz.cpp
routing/ConnectionGraph.cpp
routing/GraphImplementation.cpp
routing/GraphImplementationGroup.cpp
routing/GraphImplementationComplete.cpp
routing/GraphImplementationDeBruijn.cpp
routing/GraphImplementationRandom.cpp
scheduling/SwitchMan_adapters.cpp
scheduling/TaskCreator.cpp
scheduling/SwitchMan.cpp
scheduling/VirtualProcessor.cpp
profiling/Derivative.cpp
profiling/TimePrinter.cpp
profiling/Profiler.cpp
profiling/TickLogger.cpp
cryptography/crypto.cpp
structures/StaticVector.cpp
memory/ReusableMemoryStore.cpp
memory/ChunkAllocatorWithDefragmentation.cpp
memory/DefragmentationGroup.cpp
memory/RingAllocator.cpp
memory/MyAllocator.cpp
memory/allocator.cpp
memory/DefragmentationLane.cpp
plugins/CorePlugin.cpp
plugins/RegisteredPlugin.cpp
communication/VirtualCommunicator.cpp
communication/MessagesHandler.cpp
communication/MessageRouter.cpp
communication/Message.cpp
communication/mpi_tags.cpp
communication/BufferedData.cpp

)

ADD_LIBRARY( RayPlatform_SHARED SHARED ${source_files})
ADD_LIBRARY( RayPlatform_STATIC STATIC ${source_files})

SET_TARGET_PROPERTIES( RayPlatform_SHARED RayPlatform_STATIC PROPERTIES OUTPUT_NAME RayPlatform )
