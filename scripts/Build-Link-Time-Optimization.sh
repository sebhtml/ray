#!/bin/bash


mpicxx \
-Wall -ansi -O3 -march=native -flto -fwhole-program \
 -flto-report \
 -D MAXKMERLENGTH=32  \
 -D RAY_VERSION=\"2.1.0-devel\" -D RAYPLATFORM_VERSION=\"1.1.0-devel\" \
 -I. -Icode -IRayPlatform \
 $(find code/|grep .cpp$;find RayPlatform/|grep .cpp$) \
 -o Ray
