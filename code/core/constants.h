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
along with this program (COPYING).  
see <http://www.gnu.org/licenses/>

*/

#ifndef _constants
#define _constants

#include <stdint.h>

#define EXIT_NOMOREMEMORY 42

#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#define OS_WIN
#else
#define OS_POSIX
#endif

#ifdef __GNUC__
#define __STDC_FORMAT_MACROS /* for PRIu64 */
#endif

#ifdef FORCE_PACKING
#ifdef __GNUC__
#define ATTRIBUTE_PACKED  __attribute__ ((packed))
#else
#define ATTRIBUTE_PACKED
#endif
#else
#define ATTRIBUTE_PACKED
#endif


#define DUMMY_LIBRARY 40000

#define RAY_VERSION "1.6.0-devel"

#define _ENCODING_A 0
#define _ENCODING_T 1
#define _ENCODING_C 2
#define _ENCODING_G 3

// the maximum of processes is utilized to construct unique hyperfusions IDs
#define MAX_NUMBER_OF_MPI_PROCESSES 1000000
#define INVALID_RANK MAX_NUMBER_OF_MPI_PROCESSES

#define MAX_VERTICES_TO_VISIT 500
#define TIP_LIMIT 40
#define _MINIMUM_COVERAGE 2

// Open-MPI threshold if 4k (4096), and this include Open-MPI's metadata.
// tests show that 4096-100 bytes are sent eagerly, too.
// divide that by eight and you get the number of 64-bit integers 
// allowed in a eager single communication

/*
 * "4096 is rendezvous. For eager, try 4000 or lower. "
 *  --Eugene Loh  (Oracle)
 *  http://www.open-mpi.org/community/lists/devel/2010/11/8700.php
 *
 */

//#define MAXIMUM_MESSAGE_SIZE_IN_BYTES     131072 // 128 KiB
#define MAXIMUM_MESSAGE_SIZE_IN_BYTES 4000

#define MASTER_RANK 0x00

/*
 * this is the type used to store coverage values
 */
#define COVERAGE_TYPE uint16_t

#endif
