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
include plugin_*/Makefile

$(TARGET): $(obj-y)
	$(AR) rcs $@ $^

# inference rule
%.o: %.cpp
	$(MPICXX) $(CXXFLAGS) -I ../RayPlatform -I. -c -o $@ $<

clean:
	$(RM) -f ray_core.o $(PLUGINS-y) $(obj-y) $(TARGET)


