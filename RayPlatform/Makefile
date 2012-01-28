# author: Sébastien Boisvert
#
# based on http://www.ravnborg.org/kbuild/makefiles.html
#
# to build an application that uses Ray Platform
# you need to call the make with:
#
# make RAY_APPLICATION_SCRIPTS=/path/to/scripts (see example in ../Application/scripting)
#

MPICXX = mpicxx

######## <Platform>

#memory
obj-y += memory/ReusableMemoryStore.o memory/MyAllocator.o memory/RingAllocator.o \
memory/malloc_types.o 
obj-y += memory/allocator.o
obj-y += memory/DefragmentationGroup.o memory/ChunkAllocatorWithDefragmentation.o memory/DefragmentationLane.o

# routing stuff for option -route-messages
#
obj-y += routing/GraphImplementation.o
obj-y += routing/GraphImplementationRandom.o
obj-y += routing/GraphImplementationComplete.o
obj-y += routing/GraphImplementationDeBruijn.o
obj-y += routing/GraphImplementationKautz.o
obj-y += routing/GraphImplementationExperimental.o
obj-y += routing/GraphImplementationGroup.o
obj-y += routing/ConnectionGraph.o

#communication
obj-y += communication/mpi_tags.o communication/VirtualCommunicator.o communication/BufferedData.o \
communication/Message.o  communication/MessagesHandler.o
obj-y += communication/MessageRouter.o

# scheduling stuff
obj-y += scheduling/VirtualProcessor.o
obj-y += scheduling/TaskCreator.o
obj-y += scheduling/SwitchMan.o
obj-y += scheduling/SwitchMan_adapters.o

#core
obj-y += core/slave_modes.o 
obj-y += core/OperatingSystem.o
obj-y += core/master_modes.o
obj-y += core/ComputeCore.o
obj-y += core/statistics.o

# plugin architecture

obj-y += plugins/CorePlugin.o
obj-y += plugins/RegisteredPlugin.o

# structures
obj-y += structures/StaticVector.o 

# profiling
obj-y += profiling/Profiler.o
obj-y += profiling/Derivative.o
obj-y += profiling/TickLogger.o
obj-y += profiling/TimePrinter.o

# handlers

obj-y += handlers/SlaveModeHandler.o
obj-y += handlers/MasterModeHandler.o
obj-y += handlers/MessageTagHandler.o

#cryptography
obj-y += cryptography/crypto.o

######## </Platform>

# set RAY_APPLICATION_SCRIPTS to the scripts
# of Ray Application if none are provided
ifndef $(RAY_APPLICATION_SCRIPTS)
	RAY_APPLICATION_SCRIPTS=../code/scripting
endif

ifndef $(RAY_PLATFORM)
	RAY_PLATFORM=.
endif

all: libRayPlatform.a

libRayPlatform.a: $(obj-y)
	$(AR) rcs  libRayPlatform.a $(obj-y)
	@echo "RAY_APPLICATION_SCRIPTS= $(RAY_APPLICATION_SCRIPTS)"

# inference rule
#@echo "  MPICXX" $<
%.o: %.cpp
	$(MPICXX) -c -o $@ $<  $(CXXFLAGS) -I$(RAY_APPLICATION_SCRIPTS) -I$(RAY_PLATFORM)

clean:
	@rm -f libRayPlatform.a $(obj-y)
	@echo CLEAN

