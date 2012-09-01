#!/bin/bash


mpicxx \
-Wall -std=c++98 -O3 -march=native -flto -fwhole-program \
 -D HAVE_LIBZ -D HAVE_LIBBZ2 -lz -lbz2 \
 -flto-report \
 -D MAXKMERLENGTH=32  \
 -D RAY_VERSION=\"2.1.0-devel\" -D RAYPLATFORM_VERSION=\"1.1.0-devel\" \
 -I. -Icode -IRayPlatform \
 $(find code/|grep .cpp$;find RayPlatform/|grep .cpp$) \
 -o Ray

# remove even more stuff from the executable
strip Ray
