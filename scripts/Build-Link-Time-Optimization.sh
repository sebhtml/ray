#!/bin/bash


mpicxx \
-Wall -std=c++98 -Os -march=native -flto -fwhole-program \
 -D CONFIG_HAVE_LIBZ -D CONFIG_HAVE_LIBBZ2 -lz -lbz2 \
 -flto-report \
 -D CONFIG_MAXKMERLENGTH=32  \
 -D CONFIG_RAY_VERSION=\"x.y.z\" -D CONFIG_RAYPLATFORM_VERSION=\"x.y.z\" \
 -I. -I RayPlatform  \
 $(find code/|grep .cpp$;find RayPlatform/|grep .cpp$) \
 -o Ray

# remove even more stuff from the executable
strip Ray
