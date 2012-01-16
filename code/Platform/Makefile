
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
obj-y += scripting/ScriptEngine.o

#core
obj-y += core/slave_modes.o 
obj-y += core/OperatingSystem.o
obj-y += core/master_modes.o
obj-y += core/ComputeCore.o
obj-y += core/statistics.o

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

######## </Platform>

all: libRayPlatform.a

libRayPlatform.a: $(obj-y)
	$(AR) rcs  libRayPlatform.a $(obj-y)

# inference rule
#@echo "  MPICXX" $<
%.o: %.cpp
	$(MPICXX) -c -o $@ $<  $(CXXFLAGS) -I../Application/scripting -I.

clean:
	@rm -f libRayPlatform.a $(obj-y)
	@echo CLEAN

