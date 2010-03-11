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
#include<stdint.h>
#include<SplayTree.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<string>
#include<vector>
using namespace std;

#define _SEGMENT_LENGTH 7

// the maximum of processes is utilized to construct unique hyperfusions IDs
#define MAX_NUMBER_OF_MPI_PROCESSES 10000



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
 * convert a char* to uint64_t
 * A=00, T=01, C=10, G=11
 */
uint64_t wordId(const char*a,bool colorSpace);

/*
 * convert a 64-bit integer to a string
 */
string idToWord(uint64_t i,int wordSize);

/*
 * verify that x has only A,T,C, and G
 */
bool isValidDNA(const char*x);

/*
 * get the first letter of a uint64_t
 */
char getFirstSymbol(uint64_t i,int k);

/*
 * get the last letter of a uint64_t
 */
char getLastSymbol(uint64_t i,int w);

/*
 * output in stdout the binary view of a 64-bit integer.
 */
void coutBIN(uint64_t a);
void coutBIN8(uint8_t a);

/*
 * get the prefix
 */
uint64_t getKPrefix(uint64_t a,int k);

/*
 * get the suffix
 */
uint64_t getKSuffix(uint64_t a,int k);

/*
 * complement a vertex, and return another one
 */
uint64_t complementVertex(uint64_t a,int m_wordSize,bool useColorSpace);

/*
 * add line breaks to a string
 */
string addLineBreaks(string sequence);

/*
 * hash a uint64_t in a uniform way.
 */
uint64_t hash_uint64_t(uint64_t a);

void*__Malloc(int c);
void __Free(void*a);


/*
 * compute the reverse complement in color space (it is just the same, but reverse)
 */

uint64_t complementVertex_colorSpace(uint64_t a,int b);

/*
 * complement vertex, normal.
 */
uint64_t complementVertex_normal(uint64_t a,int m_wordSize);

/*
 * use mini distant segments here.
 */
uint64_t wordId_DistantSegments(const char*a);

uint8_t getFirstSegmentLastCode(uint64_t v,int segmentLength,int totalLength);
uint8_t getFirstSegmentFirstCode(uint64_t v,int segmentLength,int totalLength);
uint8_t getSecondSegmentFirstCode(uint64_t v,int segmentLength,int totalLength);
uint8_t getSecondSegmentLastCode(uint64_t v,int segmentLength,int totalLength);

uint8_t charToCode(char a);

char codeToChar(uint8_t a);


#endif
