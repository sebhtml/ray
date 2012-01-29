# author: Sébastien Boisvert
#
# based on http://www.ravnborg.org/kbuild/makefiles.html
#

include common.mk

all: libRayPlatform.a

libRayPlatform.a: $(obj-y)
	$(AR) rcs  libRayPlatform.a $(obj-y)

# inference rule
%.o: %.cpp
	$(MPICXX) $(CXXFLAGS) -I. -c -o $@ $<

clean:
	$(RM) -f libRayPlatform.a $(obj-y)

