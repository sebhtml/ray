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
along with this program (COPYING).  
see <http://www.gnu.org/licenses/>

*/

#ifndef _common_functions
#define _common_functions

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#define MAXIMUM_NUMBER_OF_LIBRARIES 250
#define DUMMY_LIBRARY 255

#define RAY_VERSION "1.2.4"

#include<master_modes.h>
#include<slave_modes.h>
#include<mpi_tags.h>
#include<stdint.h>
#include<stdlib.h>
#include<sys/types.h>
#include<time.h>
#include<unistd.h>
#include<string>
#include<vector>
using namespace std;

#define SHOW_PROGRESS

// the maximum of processes is utilized to construct unique hyperfusions IDs
#define MAX_NUMBER_OF_MPI_PROCESSES 1000000

#define __max(a,b) (((a)>(b)) ? (a) : (b))

#define MAX_DEPTH 60
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

#define MASTER_RANK 0x0

#define _SEGMENT_LENGTH 5

/*
 * this is the type used to store coverage values
 * default is 8 bits, unsigned.
 */
#define COVERAGE_TYPE uint8_t

/*
 *  complement the sequence of a biological thing
 */
string reverseComplement(string a,char*rev);

/*
 * convert a char* to uint64_t
 * A=00, T=01, C=10, G=11
 */
uint64_t wordId(const char*a);
uint64_t wordId_Classic(const char*a);

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

void*__Malloc(int c);
void __Free(void*a);
void*__Realloc(void*a,int c);

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

string convertToString(vector<uint64_t>*b,int m_wordSize);

int vertexRank(uint64_t a,int _size,int w);

uint64_t kmerAtPosition(char*string,int pos,int w,char strand,bool color);

int roundNumber(int number,int alignment);

uint64_t getMicroSeconds();

void showMemoryUsage(int rank);

vector<uint64_t> _getOutgoingEdges(uint64_t a,uint8_t edges,int k);
vector<uint64_t> _getIngoingEdges(uint64_t a,uint8_t edges,int k);

char complementNucleotide(char c);

uint64_t hash_function_1(uint64_t a,int w);
uint64_t hash_function_2(uint64_t a,int w,uint64_t*b);

#endif


