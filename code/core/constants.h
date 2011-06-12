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

#define RAY_VERSION "1.6.0-rc2"

#include <stdint.h>

/* exit codes */

/* EXIT_SUCCESS 0 (defined in stdlib.h) */
#define EXIT_NOMOREMEMORY 42

/*
 * Detect the operating system
 */
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__) 
#define OS_WIN
#else
#define OS_POSIX
#endif

/*
* - * Define the inline keyword if using 
*   - * Microsoft Visual C++
*   - */
#if defined(__GNUC__)
#define INLINE inline
#elif defined(_MSC_VER)
#define INLINE __forceinline
#else
#define INLINE
#endif

/*
 * If GNU, must set __STDC_FORMAT_MACROS
 * to get PRIu64 definition
 */
#ifdef __GNUC__
#define __STDC_FORMAT_MACROS /* for PRIu64 */
#endif

/*
 * Include those libraries for Microsoft Visual C++
 */
#ifdef OS_WIN
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

/* the maximum of processes is utilized to construct unique hyperfusions IDs */
#define MAX_NUMBER_OF_MPI_PROCESSES 1000000
#define INVALID_RANK MAX_NUMBER_OF_MPI_PROCESSES

#define MAX_VERTICES_TO_VISIT 500
#define TIP_LIMIT 40
#define _MINIMUM_COVERAGE 2

/*
 Open-MPI threshold if 4k (4096), and this include Open-MPI's metadata.
 tests show that 4096-100 bytes are sent eagerly, too.
 divide that by eight and you get the number of 64-bit integers 
 allowed in a eager single communication

 * "4096 is rendezvous. For eager, try 4000 or lower. "
 *  --Eugene Loh  (Oracle)
 *  http://www.open-mpi.org/community/lists/devel/2010/11/8700.php
 *
 */

#define MAXIMUM_MESSAGE_SIZE_IN_BYTES 4000

#define MASTER_RANK 0

/*
 * this is the type used to store coverage values
 */
#define COVERAGE_TYPE uint16_t

#endif
