VERSION = 2
PATCHLEVEL = 3
SUBLEVEL = 0
EXTRAVERSION =
NAME = Distributed Pumpkin

# Like in Linux, stuff that must be propagated to
# the source code (C++ here) or other Makefiles
# are prefixed with CONFIG_

# author: Sébastien Boisvert
# Makefile for the ray assembler
# Objects appended to obj-y are compiled and linked.
# Objects appended to obj-n are not compiled and linked.
#
# Based on http://www.ravnborg.org/kbuild/makefiles.html
#
# The code is distributed in a small Ray core that is built on top
# of the RayPlatform.
#
# Then, plugins (interface CorePlugin in the RayPlatform)
# are simply added onto the core (class ComputeCore in the
# RayPlatform).

# this can be changed with make MPICXX=...
MPICXX = mpicxx

# CXXFLAGS can be changed by the end user with make CXXFLAGS="..."
CXXFLAGS = -O3 -std=c++98 -Wall -g

RM = rm
CD = cd
MAKE = make
ECHO = echo
MKDIR = mkdir
CP = cp

CONFIG_RAY_VERSION = $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)

#######################################################################
# Compilation options
#######################################################################

# installation prefix with make install
PREFIX=install-prefix

# maximum k-mer length
# in nucleotides
# 32 nucleotides are stored on 1x64 bits
# 45 nucleotides are stored on 2x64 bits
# 64 nucleotides are stored on 2x64 bits
# 96 nucleotides are stored on 3x64 bits
# Intermediate values should utilise intermediate numbers of bits.
# There is no maximum value for MAXKMERLENGTH
MAXKMERLENGTH = 32

# support for .gz files
# needs libz
# set to no if you don't have libz
# y/n
HAVE_LIBZ = n

# support for .bz2 files
# needs libbz2
# set to no if you don't have libbz2
# y/n
HAVE_LIBBZ2 = n

# pack structures to reduce memory usage
# will work on x86 and x86_64
# won't work on Itanium and on Sparc
# The operating system's kernel must also allow the retrieval of 8-byte chunks sitting
# between two memory cache pages. Linux can do that, but I don't know for others.
# if you are not sure, type uname -a to get your processor architecture.
#
# Seems to fail or be very slow on Intel Xeon too.
#
# y/n
FORCE_PACKING = n

# compile assertions
# Ray may be faster when ASSERT=n
# y/n
ASSERT = n

# collect profiling information with -run-profiler
# if set to n, the code is not even compiled in
PROFILER_COLLECT=n

# use the precision clock
# needs -l rt too
CLOCK_GETTIME=n

# use MPI I/O for file operations

MPI_IO=n

#######################################################################
# Don't edit below this this point ---> .
#######################################################################

# Build configuration options correctly for the source code

CONFIG_ASSERT=$(ASSERT)
CONFIG_HAVE_LIBZ=$(HAVE_LIBZ)
CONFIG_HAVE_LIBBZ2=$(HAVE_LIBBZ2)
CONFIG_FORCE_PACKING=$(FORCE_PACKING)
CONFIG_PROFILER_COLLECT=$(PROFILER_COLLECT)
CONFIG_CLOCK_GETTIME=$(CLOCK_GETTIME)
CONFIG_MPI_IO=$(MPI_IO)

# These 2 are used by an other Makefile
export CONFIG_HAVE_LIBZ
export CONFIG_HAVE_LIBBZ2

#######################################################################

# Build the CONFIG_FLAGS
# This could be stored in a config.h too.
CONFIG_FLAGS-y=

#maximum k-mer length
CONFIG_FLAGS-y += -D CONFIG_MAXKMERLENGTH=$(MAXKMERLENGTH)

# compile assertions
CONFIG_FLAGS-$(CONFIG_ASSERT) += -D CONFIG_ASSERT -D ASSERT

#compile with zlib
CONFIG_FLAGS-$(CONFIG_HAVE_LIBZ) += -D CONFIG_HAVE_LIBZ
LDFLAGS-$(CONFIG_HAVE_LIBZ) += -lz

#compile with libbz2
CONFIG_FLAGS-$(CONFIG_HAVE_LIBBZ2) += -D CONFIG_HAVE_LIBBZ2
LDFLAGS-$(CONFIG_HAVE_LIBBZ2) += -lbz2

# pack data in memory to save space
CONFIG_FLAGS-$(CONFIG_FORCE_PACKING) += -D CONFIG_FORCE_PACKING

# use MPI I/O
CONFIG_FLAGS-$(CONFIG_MPI_IO) += -D CONFIG_MPI_IO
CONFIG_FLAGS-$(CONFIG_PROFILER_COLLECT) += -D CONFIG_PROFILER_COLLECT
CONFIG_FLAGS-$(CONFIG_CLOCK_GETTIME) += -D CONFIG_CLOCK_GETTIME
LDFLAGS-$(CONFIG_CLOCK_GETTIME) += -l rt
CONFIG_FLAGS-y += -D CONFIG_RAY_VERSION=\"$(CONFIG_RAY_VERSION)\"

# CONFIG_FLAGS is separate from CXXFLAGS
# This eases building the package in distributions
LDFLAGS = $(LDFLAGS-y)
CONFIG_FLAGS=$(CONFIG_FLAGS-y)

Q=@

#######################################################################
# Build rules.
# the target is Ray
all: Ray

# inference rule
%.o: %.cpp
	$(Q)$(ECHO) "  CXX $@"
	$(Q)$(MPICXX) $(CXXFLAGS) $(CONFIG_FLAGS) -I. -IRayPlatform -c $< -o $@

include code/*/Makefile

showOptions:
	$(Q)echo ""
	$(Q)echo "Compilation options (you can change them of course)"
	$(Q)echo ""
	$(Q)echo PREFIX = $(PREFIX)
	$(Q)echo MPICXX = $(MPICXX)
	$(Q)echo MAXKMERLENGTH = $(MAXKMERLENGTH)
	$(Q)echo FORCE_PACKING = $(FORCE_PACKING)
	$(Q)echo ASSERT = $(ASSERT)
	$(Q)echo HAVE_LIBZ = $(HAVE_LIBZ)
	$(Q)echo HAVE_LIBBZ2 = $(HAVE_LIBBZ2)
	$(Q)echo ""
	$(Q)echo "Compilation and linking flags (generated automatically)"
	$(Q)echo ""
	$(Q)echo CXXFLAGS = $(CXXFLAGS)
	$(Q)echo CONFIG_FLAGS = $(CONFIG_FLAGS)
	$(Q)echo LDFLAGS = $(LDFLAGS)
	$(Q)echo ""
	$(Q)touch showOptions
	
# how to make Ray
Ray: code/application_core/ray_main.o libRay.a libRayPlatform.a
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(MPICXX) $^ -o$@ $(LDFLAGS)
	$(Q)$(ECHO) $(PREFIX) > PREFIX

libRay.a: $(obj-y)
	$(Q)$(ECHO) "  AR $@"
	$(Q)$(AR) rcs $@ $^

libRayPlatform.a: RayPlatform/libRayPlatform.a
	$(Q)$(CP) $^ $@

RayPlatform/libRayPlatform.a:
	$(Q)$(MAKE) $(MFLAGS) -C RayPlatform

clean:
	$(Q)$(MAKE) $(MFLAGS) -C RayPlatform clean
	$(Q)$(ECHO) CLEAN Ray plugins
	$(Q)$(RM) -f Ray PREFIX $(obj-y) libRay.a libRayPlatform.a code/application_core/ray_main.o

install:
	$(eval PREFIX=$(shell cat PREFIX))

	$(Q)$(MKDIR) -p $(PREFIX)

	$(Q)$(ECHO) ""
	$(Q)$(ECHO) "Installing Ray to $(PREFIX)"
	$(Q)$(ECHO) ""

	$(Q)cp LICENSE.txt $(PREFIX)
	$(Q)cp gpl-3.0.txt $(PREFIX)
	$(Q)cp RayPlatform/lgpl-3.0.txt $(PREFIX)

	$(Q)cp Ray $(PREFIX)
	$(Q)cp -r Documentation $(PREFIX)
	$(Q)cp README.md $(PREFIX)
	$(Q)cp MANUAL_PAGE.txt $(PREFIX)
	$(Q)cp AUTHORS $(PREFIX)
	$(Q)cp -r scripts $PREFIX

	$(Q)mkdir $(PREFIX)/RayPlatform
	$(Q)cp RayPlatform/AUTHORS $(PREFIX)/RayPlatform
	$(Q)cp RayPlatform/README $(PREFIX)/RayPlatform
	$(Q)cp -r RayPlatform/Documentation $(PREFIX)/RayPlatform

