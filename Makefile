VERSION = 2
PATCHLEVEL = 1
SUBLEVEL = 0
EXTRAVERSION =
NAME = Ancient Granularity of Epochs

# number of cores to use for compilation
J=1

RAY_VERSION = $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)

# author: Sébastien Boisvert
# Makefile for the ray assembler
# Objects appended to obj-y are compiled and linked.
# Objects appended to obj-n are not compiled and linked.
#############################################
# compilation options

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

# use Intel's compiler
# the name of the Intel MPI C++ compiler is mpiicpc
# Open-MPI and MPICH2 utilise mpic++ for the name.
# y/n
INTEL_COMPILER = n

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
CONFIG_PROFILER_COLLECT=n

# use the precision clock
# needs -l rt too
CONFIG_CLOCK_GETTIME=n

# OS detection based on git Makefile
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

# optimize
OPTIMIZE = y

# add -g to gcc
# see "Is there a downside to leaving in debug symbols in release builds?"
# http://stackoverflow.com/questions/5569644/is-there-a-downside-to-leaving-in-debug-symbols-in-release-builds
# in short: the executable is larger, but symbols are in a different section (thus the code is not slower)
DEBUG = n

# profiling
GPROF = n

ifeq ($(GPROF),y)
	OPTIMIZE = n
	FORCE_PACKING = n
endif

PEDANTIC = n

MPICXX-y = mpicxx

# mpic++ from an MPI implementation must be reachable with the PATH
# tested implementations of MPI include Open-MPI and MPICH2
CXXFLAGS = 

# optimization
CXXFLAGS-$(OPTIMIZE) += -O3

ifeq ($(INTEL_COMPILER),n)
# g++ options
ifeq ($(uname_S),Linux)
	#-std=c++98
	CXXFLAGS += -Wall -std=c++98
	CXXFLAGS-$(PEDANTIC) += -pedantic -Wextra 
endif
endif

# profiling
CXXFLAGS-$(GPROF) += -g -pg
LDFLAGS-$(GPROF) += -pg -g

# if you use Intel's mpiicpc, uncomment the following lines
MPICXX-$(INTEL_COMPILER) = mpiicpc 
CXXFLAGS-$(INTEL_COMPILER) += -DMPICH_IGNORE_CXX_SEEK -DMPICH_SKIP_MPICXX

#maximum k-mer length
CXXFLAGS-y += -D MAXKMERLENGTH=$(MAXKMERLENGTH)
CXXFLAGS-y += $(EXTRA)
LDFLAGS-y += $(EXTRA)

# compile assertions
CXXFLAGS-$(ASSERT) += -DASSERT

#compile with libz
CXXFLAGS-$(HAVE_LIBZ) += -DHAVE_LIBZ
LDFLAGS-$(HAVE_LIBZ) += -lz

#compile with libbz2
CXXFLAGS-$(HAVE_LIBBZ2) += -DHAVE_LIBBZ2 
LDFLAGS-$(HAVE_LIBBZ2) += -lbz2

#debug flag
CXXFLAGS-$(DEBUG) += -g
LDFLAGS-$(DEBUG)  += -g

# pack data in memory to save space
CXXFLAGS-$(FORCE_PACKING) += -DFORCE_PACKING

CXXFLAGS-$(CONFIG_PROFILER_COLLECT) += -D CONFIG_PROFILER_COLLECT
CXXFLAGS-$(CONFIG_CLOCK_GETTIME) += -D CONFIG_CLOCK_GETTIME
LDFLAGS-$(CONFIG_CLOCK_GETTIME) += -l rt
CXXFLAGS-y += -D RAY_VERSION=\\\"$(RAY_VERSION)\\\"

CXXFLAGS += $(CXXFLAGS-y)
LDFLAGS += $(LDFLAGS-y)

MPICXX = $(MPICXX-y)

# object files

#################################


# the target is Ray
all: Ray

showOptions: 
	@echo ""
	@echo "Compilation options (you can change them of course)"
	@echo ""
	@echo PREFIX = $(PREFIX)
	@echo MAXKMERLENGTH = $(MAXKMERLENGTH)
	@echo FORCE_PACKING = $(FORCE_PACKING)
	@echo ASSERT = $(ASSERT)
	@echo HAVE_LIBZ = $(HAVE_LIBZ)
	@echo HAVE_LIBBZ2 = $(HAVE_LIBBZ2)
	@echo INTEL_COMPILER = $(INTEL_COMPILER)
	@echo MPICXX = $(MPICXX)
	@echo GPROF = $(GPROF)
	@echo OPTIMIZE = $(OPTIMIZE)
	@echo DEBUG = $(DEBUG)
	@echo ""
	@echo "Compilation and linking flags (generated automatically)"
	@echo ""
	@echo CXXFLAGS = $(CXXFLAGS)
	@echo LDFLAGS = $(LDFLAGS)
	@echo ""
	@touch showOptions
	
# how to make Ray
Ray: showOptions RayPlatform/libRayPlatform.a code/TheRayGenomeAssembler.a
	$(MPICXX) $(LDFLAGS)  code/TheRayGenomeAssembler.a RayPlatform/libRayPlatform.a -o $@
	@echo $(PREFIX) > PREFIX
	@echo Ray > TARGETS

code/TheRayGenomeAssembler.a:
	@cd code; make MPICXX="$(MPICXX)" CXXFLAGS="$(CXXFLAGS)" -j $(J) all ; cd ..

RayPlatform/libRayPlatform.a:
	@cd RayPlatform; make MPICXX="$(MPICXX)" CXXFLAGS="$(CXXFLAGS)" -j $(J) ; cd ..

clean:
	@rm -f Ray showOptions PREFIX TARGETS
	@echo "Cleaning Ray Application"
	@(cd code; make clean; cd ..)
	@echo "Cleaning Ray Platform"
	@cd RayPlatform;make clean; cd ..
	@echo CLEAN

install: 
	@scripts/install.sh
