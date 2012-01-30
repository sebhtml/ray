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
AR = ar
RM = rm
CXXFLAGS= -O3 -Wall -ansi

TARGET=TheRayGenomeAssembler.a

all: $(TARGET) 

include application_core/Makefile
include plugin_MachineHelper/Makefile
include plugin_Library/Makefile
include plugin_FusionTaskCreator/Makefile
include plugin_NetworkTest/Makefile
include plugin_SequencesLoader/Makefile
include plugin_VerticesExtractor/Makefile
include plugin_JoinerTaskCreator/Makefile
include plugin_SeedingData/Makefile
include plugin_FusionData/Makefile
include plugin_CoverageGatherer/Makefile
include plugin_SeedExtender/Makefile
include plugin_MessageProcessor/Makefile
include plugin_Partitioner/Makefile
include plugin_SequencesIndexer/Makefile
include plugin_Amos/Makefile
include plugin_KmerAcademyBuilder/Makefile
include plugin_EdgePurger/Makefile
include plugin_Searcher/Makefile
include plugin_Scaffolder/Makefile

$(TARGET): $(obj-y)
	$(AR) rcs $@ $^

# inference rule
%.o: %.cpp
	$(MPICXX) $(CXXFLAGS) -I ../RayPlatform -I. -c -o $@ $<

clean:
	$(RM) -f ray_core.o $(PLUGINS-y) $(obj-y) $(TARGET)


