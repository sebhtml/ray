VERSION = 2
PATCHLEVEL = 1
SUBLEVEL = 1
EXTRAVERSION = -devel
NAME = Ancient Granularity of Epochs

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
CXXFLAGS = -O3 -std=c++98 -Wall

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

CONFIG_FLAGS-$(CONFIG_PROFILER_COLLECT) += -D CONFIG_PROFILER_COLLECT
CONFIG_FLAGS-$(CONFIG_CLOCK_GETTIME) += -D CONFIG_CLOCK_GETTIME
LDFLAGS-$(CONFIG_CLOCK_GETTIME) += -l rt
CONFIG_FLAGS-y += -D CONFIG_RAY_VERSION=\"$(CONFIG_RAY_VERSION)\"

# CONFIG_FLAGS is separate from CXXFLAGS
# This eases building the package in distributions
LDFLAGS = $(LDFLAGS-y)
CONFIG_FLAGS=$(CONFIG_FLAGS-y)

#######################################################################
# Build rules.
# the target is Ray
all: Ray

# inference rule
%.o: %.cpp
	@$(ECHO) "  CXX $@"
	@$(MPICXX) $(CXXFLAGS) $(CONFIG_FLAGS) -I. -c -o $@ $<

include code/application_core/Makefile
include code/plugin_*/Makefile

showOptions: 
	@echo ""
	@echo "Compilation options (you can change them of course)"
	@echo ""
	@echo PREFIX = $(PREFIX)
	@echo MPICXX = $(MPICXX)
	@echo MAXKMERLENGTH = $(MAXKMERLENGTH)
	@echo FORCE_PACKING = $(FORCE_PACKING)
	@echo ASSERT = $(ASSERT)
	@echo HAVE_LIBZ = $(HAVE_LIBZ)
	@echo HAVE_LIBBZ2 = $(HAVE_LIBBZ2)
	@echo ""
	@echo "Compilation and linking flags (generated automatically)"
	@echo ""
	@echo CXXFLAGS = $(CXXFLAGS)
	@echo CONFIG_FLAGS = $(CONFIG_FLAGS)
	@echo LDFLAGS = $(LDFLAGS)
	@echo ""
	@touch showOptions
	
# how to make Ray
Ray: showOptions RayPlatform/libRayPlatform.a $(obj-y)
	@$(ECHO) "  LD $@"
	@$(MPICXX) $(obj-y) RayPlatform/libRayPlatform.a -o $@ $(LDFLAGS)
	@$(ECHO) $(PREFIX) > PREFIX

RayPlatform/libRayPlatform.a:
	@$(CD) RayPlatform; $(MAKE) $(MFLAGS) ; $(CD) ..

clean:
	@$(CD) RayPlatform; $(MAKE) clean; $(CD) ..
	@$(ECHO) CLEAN Ray plugins
	@$(RM) -f Ray showOptions PREFIX $(obj-y)

install: 
	$(eval PREFIX=$(shell cat PREFIX))

	@$(MKDIR) -p $(PREFIX)

	@$(ECHO) ""
	@$(ECHO) "Installing Ray to $(PREFIX)"
	@$(ECHO) ""

	@cp LICENSE.txt $(PREFIX)
	@cp gpl-3.0.txt $(PREFIX)
	@cp RayPlatform/lgpl-3.0.txt $(PREFIX)

	@cp Ray $(PREFIX)
	@cp -r Documentation $(PREFIX)
	@cp README.md $(PREFIX)
	@cp MANUAL_PAGE.txt $(PREFIX)
	@cp AUTHORS $(PREFIX)
	@cp -r scripts $PREFIX

	@mkdir $(PREFIX)/RayPlatform
	@cp RayPlatform/AUTHORS $(PREFIX)/RayPlatform
	@cp RayPlatform/README $(PREFIX)/RayPlatform
	@cp -r RayPlatform/Documentation $(PREFIX)/RayPlatform

