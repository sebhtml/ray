/*
Ray
Copyright (C)  2011  Sébastien Boisvert

http://DeNovoAssembler.SourceForge.Net/

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You have received a copy of the GNU General Public License
along with this program (gpl-3.0.txt).  
see <http://www.gnu.org/licenses/>

*/

#ifndef _constants
#define _constants

#ifndef RAY_VERSION
#define RAY_VERSION "Unknown"
#endif

/* 
 * Define the maximum k-mer length when
 * the compiler/make does not.
 */
#ifndef MAXKMERLENGTH
#define MAXKMERLENGTH 32
#endif

// some multipliers

/** something occuring twice is repeated */
#define REPEAT_MULTIPLIER 2

/** 3 standard deviations on both sides of a normal distribution include most points */
#define FRAGMENT_MULTIPLIER 3

#include <stdlib.h> /* for __WORDSIZE hopefully */
#include <stdint.h>

/* exit codes */

/*
 * Include those libraries for Microsoft Visual C++
 */
#ifdef _MSC_VER
#include <xiosbase>
#include <stdexcept>
/* http://msdn.microsoft.com/en-us/library/b0084kay%28VS.80%29.aspx */
#define __func__ __FUNCTION__ 
#endif

#ifdef FORCE_PACKING
/*
 * With gcc, one can pack data structures.
 */
	#ifdef __GNUC__
		#define ATTRIBUTE_PACKED  __attribute__ ((packed))
/*
 * For Microsoft Visual C++
 */
	#elif defined(_MSC_VER)
		#define ATTRIBUTE_PACKED /* sorry, not available yet */
	#else
		#define ATTRIBUTE_PACKED
	#endif
#else
	#define ATTRIBUTE_PACKED
#endif

#define DUMMY_LIBRARY 40000

#define RAY_NUCLEOTIDE_A 0 /* ~00 == 11 */
#define RAY_NUCLEOTIDE_C 1 /* ~01 == 10 */
#define RAY_NUCLEOTIDE_G 2 /* ~10 == 01 */
#define RAY_NUCLEOTIDE_T 3 /* ~11 == 00 */

#define DOUBLE_ENCODING_A_COLOR '0'
#define DOUBLE_ENCODING_C_COLOR '1'
#define DOUBLE_ENCODING_G_COLOR '2'
#define DOUBLE_ENCODING_T_COLOR '3'

/* maximum value for a uint16_t */
#define RAY_MAXIMUM_READ_LENGTH 65535 

#define MAX_VERTICES_TO_VISIT 500
#define TIP_LIMIT 40

/*
 * this is the type used to store coverage values
 *
 * possible values are:
 *
 * - uint8_t for coverage values from 0 to 256-1
 * - uint16_t for coverage values from 0 to 65536-1
 * - uint32_t for coverage values from 0 to 4294967296-1
 *
 *
 */

#ifndef CONFIG_MAXIMUM_COVERAGE
	#define CONFIG_MAXIMUM_COVERAGE 99999
#endif


#if CONFIG_MAXIMUM_COVERAGE < 256
	typedef uint8_t CoverageDepth ;
#elif CONFIG_MAXIMUM_COVERAGE < 65536
	typedef uint16_t CoverageDepth;
#elif CONFIG_MAXIMUM_COVERAGE < 4294967296
	typedef uint32_t CoverageDepth;
#else
	typedef uint64_t CoverageDepth;
#endif

/** 32-bit or 64-bit system */

#if defined(__WORDSIZE)
/** use __WORDSIZE */
#define NUMBER_OF_BITS __WORDSIZE

#elif defined(_WIN64)

#define NUMBER_OF_BITS 64

#elif defined (_WIN32)

#define NUMBER_OF_BITS 32


/** assume 64 bits */
/* you may get some compilation warnings about printf and fprintf */
#else

#define NUMBER_OF_BITS 64

#endif

/* 64-bit system */
#if NUMBER_OF_BITS == 64
#define RAY_64_BITS

/* 32-bit system */
#elif NUMBER_OF_BITS == 32
#define RAY_32_BITS

/* assume a 64-bit system */
#else
#define RAY_64_BITS
#endif

/* since Lustre is not very good at caching file input/output operations
 * Ray agglomerates these operations */

//#define CONFIG_FILE_IO_BUFFER_SIZE 1048576 /* 1 MB */
//#define CONFIG_FILE_IO_BUFFER_SIZE 4194304 /* 4 MB */
#define CONFIG_FILE_IO_BUFFER_SIZE 16777216 /* 16 MB */
//#define CONFIG_FILE_IO_BUFFER_SIZE 33554432 /* 32 MB */
//#define CONFIG_FILE_IO_BUFFER_SIZE 134217728 /* 128 MB */

/* the identifier of a read */
typedef uint64_t ReadHandle;

/* the identifier for a path in the de Bruijn graph */
typedef uint64_t PathHandle;

/* a DNA strand */
typedef char Strand;

/* a datatype for counts */
typedef uint64_t LargeCount;

/* a datatype for an index */
typedef uint64_t LargeIndex;


#endif
