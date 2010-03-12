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
along with this program (gpl-3.0.txt).  
see <http://www.gnu.org/licenses/>

*/

#ifndef _common_functions
#define _common_functions

#define VERTEX_TYPE uint64_t
#include<stdint.h>
#include<SplayTree.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<string>
#include<vector>
using namespace std;

#define _SEGMENT_LENGTH 5


// the maximum of processes is utilized to construct unique hyperfusions IDs
#define MAX_NUMBER_OF_MPI_PROCESSES 10000

// unlock the power of Ray here!
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

char codeToChar(uint8_t a);


#endif
