/*
Ray
Copyright (C)  2010  Sébastien Boisvert

http://DeNovoAssembler.SourceForge.Net/

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You have received a copy of the GNU General Public License
along with this program (LICENSE).  
see <http://www.gnu.org/licenses/>

*/

#ifndef _common_functions
#define _common_functions

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#ifdef HAVE_ZLIB_H
#include<zlib.h>
#endif

#include<master_modes.h>
#include<slave_modes.h>
#include<mpi_tags.h>

#define SHOW_PROGRESS
#define WRITE_COVERAGE_DISTRIBUTION

#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define MAX_U32 4294967295
#define MAX_U16 65535

#define _REPEATED_LENGTH_ALARM_THRESHOLD 100

#define VERTEX_TYPE u64
#include<stdint.h>
#include<SplayTree.h>
#include<stdlib.h>
#include<sys/types.h>
#include<time.h>
#include<unistd.h>
#include<string>
#include<vector>
using namespace std;

#define __max(a,b) (((a)>(b)) ? (a) : (b))

#define MAX_DEPTH 200
#define MAX_VERTICES_TO_VISIT 500
#define TIP_LIMIT 40
#define _MINIMUM_COVERAGE 2

// Open-MPI threshold if 4k (4096), and this include Open-MPI's metadata.
// tests show that 4096-100 bytes are sent eagerly, too.
// divide that by eight and you get the number of 64-bit integers 
// allowed in a eager single communication
#define _SINGLE_ELEMENT 1
#define MPI_BTL_SM_EAGER_LIMIT 0x1000 // 4096 bytes
#define _FILL_UP MPI_BTL_SM_EAGER_LIMIT/sizeof(VERTEX_TYPE)
#define MAX_UINT64_T_PER_MESSAGE _FILL_UP



#define MASTER_RANK 0x0



#define _FOREST_SIZE 0x1000

// allocators size
// for MPI communications, memory is allocated and freed with OUTBOX_ALLOCATOR_CHUNK_SIZE and INBOX_ALLOCATOR_CHUNK_SIZE
// persistant memory are stored with PERSISTENT_ALLOCATOR_CHUNK_SIZE
#define OUTBOX_ALLOCATOR_CHUNK_SIZE        0x1000000
#define DISTRIBUTION_ALLOCATOR_CHUNK_SIZE  0x1000000
#define INBOX_ALLOCATOR_CHUNK_SIZE         0x1000000
#define PERSISTENT_ALLOCATOR_CHUNK_SIZE    0x1000000

#define _SEGMENT_LENGTH 5

/*
 * this is the type used to store coverage values
 * default is 8 bits, unsigned.
 */
#define COVERAGE_TYPE unsigned char


// the maximum of processes is utilized to construct unique hyperfusions IDs
#define MAX_NUMBER_OF_MPI_PROCESSES 10000

// unlock the power of Ray here!
// currently not working...
//#define USE_DISTANT_SEGMENTS_GRAPH

// uniform random numbers
// from Heng Li's code (LGPL)
// http://maq.svn.sourceforge.net/viewvc/maq/trunk/maq/genran.h?view=markup

#ifdef _WIN32
#define ran_seed() srand(time(NULL))
#define ran_uniform() rand()/(RAND_MAX+0.0)
#else
#define ran_seed() srand48(time(NULL)*(long)getpid())
#define ran_uniform() drand48()
#endif


/*
 *  complement the sequence of a biological thing
 */
string reverseComplement(string a);

/*
 * convert a char* to VERTEX_TYPE
 * A=00, T=01, C=10, G=11
 */
VERTEX_TYPE wordId(const char*a);
VERTEX_TYPE wordId_Classic(const char*a);

/*
 * convert a 64-bit integer to a string
 */
string idToWord(VERTEX_TYPE i,int wordSize);

/*
 * verify that x has only A,T,C, and G
 */
bool isValidDNA(const char*x);

/*
 * get the first letter of a VERTEX_TYPE
 */
char getFirstSymbol(VERTEX_TYPE i,int k);

/*
 * get the last letter of a VERTEX_TYPE
 */
char getLastSymbol(VERTEX_TYPE i,int w);

/*
 * output in stdout the binary view of a 64-bit integer.
 */
void coutBIN(VERTEX_TYPE a);
void coutBIN8(uint8_t a);

/*
 * get the prefix
 */
VERTEX_TYPE getKPrefix(VERTEX_TYPE a,int k);

/*
 * get the suffix
 */
VERTEX_TYPE getKSuffix(VERTEX_TYPE a,int k);

/*
 * complement a vertex, and return another one
 */
VERTEX_TYPE complementVertex(VERTEX_TYPE a,int m_wordSize,bool useColorSpace);

/*
 * add line breaks to a string
 */
string addLineBreaks(string sequence);

/*
 * hash a VERTEX_TYPE in a uniform way.
 */
VERTEX_TYPE hash_VERTEX_TYPE(VERTEX_TYPE a);

void*__Malloc(int c);
void __Free(void*a);


/*
 * compute the reverse complement in color space (it is just the same, but reverse)
 */

VERTEX_TYPE complementVertex_colorSpace(VERTEX_TYPE a,int b);

/*
 * complement vertex, normal.
 */
VERTEX_TYPE complementVertex_normal(VERTEX_TYPE a,int m_wordSize);

/*
 * use mini distant segments here.
 */
VERTEX_TYPE wordId_DistantSegments(const char*a);

uint8_t getFirstSegmentLastCode(VERTEX_TYPE v,int segmentLength,int totalLength);
uint8_t getFirstSegmentFirstCode(VERTEX_TYPE v,int segmentLength,int totalLength);
uint8_t getSecondSegmentFirstCode(VERTEX_TYPE v,int segmentLength,int totalLength);
uint8_t getSecondSegmentLastCode(VERTEX_TYPE v,int segmentLength,int totalLength);

uint8_t charToCode(char a);

u32 hash6432(u64 key);

char codeToChar(uint8_t a);

string convertToString(vector<VERTEX_TYPE>*b,int m_wordSize);


int vertexRank(VERTEX_TYPE a,int _size);

void showProgress(time_t m_lastTime);

char*__basename(char*a);


VERTEX_TYPE kmerAtPosition(const char*string,int pos,int w,char strand,bool color);

int roundNumber(int number,int alignment);


u64 getMicroSeconds();

#endif

