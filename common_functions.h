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
#include<time.h>
#include<unistd.h>
#include<string>
#include<vector>
using namespace std;


#define __PAIRED_MULTIPLIER 2
#define __SINGLE_MULTIPLIER 2

#define MAX_DEPTH 200
#define MAX_VERTICES_TO_VISIT 500
#define TIP_LIMIT 40
#define _MINIMUM_COVERAGE 2

// Open-MPI threshold if 4k (4096), and this include Open-MPI's metadata.
// tests show that 4096-100 bytes are sent eagerly, too.
// divide that by eight and you get the number of 64-bit integers 
// allowed in a eager single communication
#define _SINGLE_ELEMENT 1
#define _FILL_UP (4096-100)/8
#define MAX_UINT64_T_PER_MESSAGE _FILL_UP

// tags
// these are the message types used by Ray
// Ray instances like to communicate a lots!
#define TAG_WELCOME 0
#define TAG_SEND_SEQUENCE 1
#define TAG_SEQUENCES_READY 2
#define TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS 3
#define TAG_VERTICES_DATA 4
#define TAG_VERTICES_DISTRIBUTED 5
#define TAG_VERTEX_PTR_REQUEST 6
#define TAG_OUT_EDGE_DATA_WITH_PTR 7
#define TAG_OUT_EDGES_DATA 8
#define TAG_SHOW_VERTICES 9
#define TAG_START_VERTICES_DISTRIBUTION 10
#define TAG_EDGES_DISTRIBUTED 11
#define TAG_IN_EDGES_DATA 12
#define TAG_IN_EDGE_DATA_WITH_PTR 13
#define TAG_START_EDGES_DISTRIBUTION 14
#define TAG_START_EDGES_DISTRIBUTION_ASK 15
#define TAG_START_EDGES_DISTRIBUTION_ANSWER 16
#define TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION 17
#define TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER 18
#define TAG_PREPARE_COVERAGE_DISTRIBUTION 19
#define TAG_COVERAGE_DATA 20
#define TAG_COVERAGE_END 21
#define TAG_SEND_COVERAGE_VALUES 22
#define TAG_READY_TO_SEED 23
#define TAG_START_SEEDING 24
#define TAG_REQUEST_VERTEX_COVERAGE 25
#define TAG_REQUEST_VERTEX_COVERAGE_REPLY 26
#define TAG_REQUEST_VERTEX_KEY_AND_COVERAGE 28
#define TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY 30
#define TAG_REQUEST_VERTEX_OUTGOING_EDGES 31
#define TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY 32
#define TAG_SEEDING_IS_OVER 33
#define TAG_GOOD_JOB_SEE_YOU_SOON 34
#define TAG_I_GO_NOW 35
#define TAG_SET_WORD_SIZE 36
#define TAG_MASTER_IS_DONE_ATTACHING_READS 37
#define TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY 38
#define TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER 39
#define TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY 40
#define TAG_REQUEST_VERTEX_INGOING_EDGES 41
#define TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY 42
#define TAG_EXTENSION_IS_DONE 43
#define TAG_ASK_EXTENSION 44
#define TAG_ASK_IS_ASSEMBLED 45
#define TAG_ASK_REVERSE_COMPLEMENT 46
#define TAG_REQUEST_VERTEX_POINTER 47
#define TAG_ASK_IS_ASSEMBLED_REPLY 48
#define TAG_MARK_AS_ASSEMBLED 49
#define TAG_ASK_EXTENSION_DATA 50
#define TAG_EXTENSION_DATA 51
#define TAG_EXTENSION_END 52
#define TAG_EXTENSION_DATA_END 53
#define TAG_ATTACH_SEQUENCE 54
#define TAG_REQUEST_READS 55
#define TAG_REQUEST_READS_REPLY 56
#define TAG_ASK_READ_VERTEX_AT_POSITION 57
#define TAG_ASK_READ_VERTEX_AT_POSITION_REPLY 58
#define TAG_ASK_READ_LENGTH 59
#define TAG_ASK_READ_LENGTH_REPLY 60
#define TAG_SAVE_WAVE_PROGRESSION 61
#define TAG_COPY_DIRECTIONS (sizeof(VERTEX_TYPE)*8-2)
#define TAG_ASSEMBLE_WAVES 63
#define TAG_SAVE_WAVE_PROGRESSION_REVERSE 65
#define TAG_ASSEMBLE_WAVES_DONE 66
#define TAG_START_FUSION 67
#define TAG_FUSION_DONE 68
#define TAG_ASK_VERTEX_PATHS_SIZE 69
#define TAG_ASK_VERTEX_PATHS_SIZE_REPLY 70
#define TAG_GET_PATH_LENGTH 71
#define TAG_GET_PATH_LENGTH_REPLY 72
#define TAG_CALIBRATION_MESSAGE 73
#define TAG_BEGIN_CALIBRATION 74
#define TAG_END_CALIBRATION 75
#define TAG_COMMUNICATION_STABILITY_MESSAGE 76
#define TAG_ASK_VERTEX_PATH 77
#define TAG_ASK_VERTEX_PATH_REPLY 78
#define TAG_INDEX_PAIRED_SEQUENCE 79
#define TAG_HAS_PAIRED_READ 80
#define TAG_HAS_PAIRED_READ_REPLY 81
#define TAG_GET_PAIRED_READ 82
#define TAG_GET_PAIRED_READ_REPLY 83
#define TAG_CLEAR_DIRECTIONS 84
#define TAG_CLEAR_DIRECTIONS_REPLY 85
#define TAG_FINISH_FUSIONS 86
#define TAG_FINISH_FUSIONS_FINISHED 87
#define TAG_DISTRIBUTE_FUSIONS 88
#define TAG_DISTRIBUTE_FUSIONS_FINISHED 89
#define TAG_EXTENSION_START 90
#define TAG_ELIMINATE_PATH 91
#define TAG_GET_PATH_VERTEX 92
#define TAG_GET_PATH_VERTEX_REPLY 93
#define TAG_SET_COLOR_MODE 94

#define MASTER_RANK 0


// modes
#define MODE_EXTENSION_ASK 0
#define MODE_START_SEEDING 1
#define MODE_DO_NOTHING 2
#define MODE_ASK_EXTENSIONS 3
#define MODE_SEND_EXTENSION_DATA 4
#define MODE_ASSEMBLE_WAVES 5
#define MODE_COPY_DIRECTIONS 6
#define MODE_ASSEMBLE_GRAPH 7
#define MODE_FUSION 8
#define MODE_MASTER_ASK_CALIBRATION 9
#define MODE_PERFORM_CALIBRATION 10
#define MODE_FINISH_FUSIONS 11
#define MODE_DISTRIBUTE_FUSIONS 12
#define MODE_AMOS 13


// allocators size
// for MPI communications, memory is allocated and freed with OUTBOX_ALLOCATOR_CHUNK_SIZE and INBOX_ALLOCATOR_CHUNK_SIZE
// persistant memory are stored with PERSISTENT_ALLOCATOR_CHUNK_SIZE
#define SIZE_10MB 10*1024*1024
#define OUTBOX_ALLOCATOR_CHUNK_SIZE SIZE_10MB
#define DISTRIBUTION_ALLOCATOR_CHUNK_SIZE SIZE_10MB
#define INBOX_ALLOCATOR_CHUNK_SIZE SIZE_10MB
#define PERSISTENT_ALLOCATOR_CHUNK_SIZE SIZE_10MB

#define CALIBRATION_DURATION 10



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

char codeToChar(uint8_t a);

string convertToString(vector<VERTEX_TYPE>*b,int m_wordSize);


int vertexRank(VERTEX_TYPE a,int _size);

void showProgress(time_t m_lastTime);

char*__basename(char*a);

#endif
