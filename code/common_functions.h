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


// tags
// these are the message types used by Ray
// Ray instances like to communicate a lots!
#define TAG_WELCOME 						0x00
#define TAG_SEND_SEQUENCE 					0x01
#define TAG_SEQUENCES_READY 					0x02
#define TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS 	0x03
#define TAG_VERTICES_DATA 					0x04
#define TAG_VERTICES_DISTRIBUTED 				0x05
#define TAG_VERTEX_PTR_REQUEST 					0x06
#define TAG_OUT_EDGE_DATA_WITH_PTR 				0x07
#define TAG_OUT_EDGES_DATA 					0x08
#define TAG_SHOW_VERTICES 0x9
#define TAG_START_VERTICES_DISTRIBUTION 0xa
#define TAG_EDGES_DISTRIBUTED 0xb
#define TAG_IN_EDGES_DATA 0xc
#define TAG_IN_EDGE_DATA_WITH_PTR 0xd
#define TAG_START_EDGES_DISTRIBUTION 0xe
#define TAG_START_EDGES_DISTRIBUTION_ASK 0xf
#define TAG_START_EDGES_DISTRIBUTION_ANSWER 0x10
#define TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION 0x11
#define TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER 0x12
#define TAG_PREPARE_COVERAGE_DISTRIBUTION 0x13
#define TAG_COVERAGE_DATA 0x14
#define TAG_COVERAGE_END 0x15
#define TAG_SEND_COVERAGE_VALUES 0x16
#define TAG_READY_TO_SEED 0x17
#define TAG_START_SEEDING 0x18
#define TAG_REQUEST_VERTEX_COVERAGE 0x19
#define TAG_REQUEST_VERTEX_COVERAGE_REPLY 0x1a
#define TAG_REQUEST_VERTEX_KEY_AND_COVERAGE 0x1b
#define TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY 0x1c
#define TAG_REQUEST_VERTEX_OUTGOING_EDGES 0x1d
#define TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY 0x1e
#define TAG_SEEDING_IS_OVER 0x1f
#define TAG_GOOD_JOB_SEE_YOU_SOON 0x20
#define TAG_I_GO_NOW 0x21
#define TAG_SET_WORD_SIZE 0x22
#define TAG_MASTER_IS_DONE_ATTACHING_READS 0x23
#define TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY 0x24
#define TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER 0x25
#define TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY 0x26
#define TAG_REQUEST_VERTEX_INGOING_EDGES 0x27
#define TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY 0x28
#define TAG_EXTENSION_IS_DONE 0x29
#define TAG_ASK_EXTENSION 0x2a
#define TAG_ASK_IS_ASSEMBLED 0x2b
#define TAG_ASK_REVERSE_COMPLEMENT 0x2c
#define TAG_REQUEST_VERTEX_POINTER 0x2d
#define TAG_ASK_IS_ASSEMBLED_REPLY 0x2e
#define TAG_MARK_AS_ASSEMBLED 0x2f
#define TAG_ASK_EXTENSION_DATA 0x30
#define TAG_EXTENSION_DATA 0x31
#define TAG_EXTENSION_END 0x32
#define TAG_EXTENSION_DATA_END 0x33
#define TAG_ATTACH_SEQUENCE 0x34
#define TAG_REQUEST_READS 0x35
#define TAG_REQUEST_READS_REPLY 0x36
#define TAG_ASK_READ_VERTEX_AT_POSITION 0x37
#define TAG_ASK_READ_VERTEX_AT_POSITION_REPLY 0x38
#define TAG_ASK_READ_LENGTH 0x39
#define TAG_ASK_READ_LENGTH_REPLY 0x3a
#define TAG_SAVE_WAVE_PROGRESSION 0x3b
#define TAG_COPY_DIRECTIONS 0x3c
#define TAG_ASSEMBLE_WAVES 0x3d
#define TAG_SAVE_WAVE_PROGRESSION_REVERSE 0x3e
#define TAG_ASSEMBLE_WAVES_DONE 0x3f
#define TAG_START_FUSION 0x40
#define TAG_FUSION_DONE 0x41
#define TAG_ASK_VERTEX_PATHS_SIZE 0x42
#define TAG_ASK_VERTEX_PATHS_SIZE_REPLY 0x43
#define TAG_GET_PATH_LENGTH 0x44
#define TAG_GET_PATH_LENGTH_REPLY 0x45
#define TAG_CALIBRATION_MESSAGE 0x46
#define TAG_BEGIN_CALIBRATION 0x47
#define TAG_END_CALIBRATION 0x48
#define TAG_COMMUNICATION_STABILITY_MESSAGE 0x49
#define TAG_ASK_VERTEX_PATH 0x4a
#define TAG_ASK_VERTEX_PATH_REPLY 0x4b
#define TAG_INDEX_PAIRED_SEQUENCE 0x4c
#define TAG_HAS_PAIRED_READ 0x4d
#define TAG_HAS_PAIRED_READ_REPLY 0x4e
#define TAG_GET_PAIRED_READ 0x4f
#define TAG_GET_PAIRED_READ_REPLY 0x50
#define TAG_CLEAR_DIRECTIONS 0x51
#define TAG_CLEAR_DIRECTIONS_REPLY 0x52
#define TAG_FINISH_FUSIONS 0x53
#define TAG_FINISH_FUSIONS_FINISHED 0x54
#define TAG_DISTRIBUTE_FUSIONS 0x55
#define TAG_DISTRIBUTE_FUSIONS_FINISHED 0x56
#define TAG_EXTENSION_START 0x57
#define TAG_ELIMINATE_PATH 0x58
#define TAG_GET_PATH_VERTEX 0x59
#define TAG_GET_PATH_VERTEX_REPLY 0x5a
#define TAG_SET_COLOR_MODE 0x5b
#define TAG_AUTOMATIC_DISTANCE_DETECTION 0x5c
#define TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE 0x5d
#define TAG_LIBRARY_DISTANCE 0x5e
#define TAG_ASK_LIBRARY_DISTANCES 0x5f
#define TAG_ASK_LIBRARY_DISTANCES_FINISHED 0x60
#define TAG_UPDATE_LIBRARY_INFORMATION 0x61
#define TAG_RECEIVED_COVERAGE_INFORMATION 0x62
#define TAG_REQUEST_READ_SEQUENCE 0x63
#define TAG_REQUEST_READ_SEQUENCE_REPLY 0x64

#define MASTER_RANK 0x0

// master modes
#define MASTER_MODE_LOAD_CONFIG 0x0
#define MASTER_MODE_LOAD_SEQUENCES 0x1
#define MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION 0x2
#define MASTER_MODE_SEND_COVERAGE_VALUES 0x3
#define MASTER_MODE_TRIGGER_EDGES_DISTRIBUTION 0x4
#define MASTER_MODE_START_EDGES_DISTRIBUTION 0x5
#define MASTER_MODE_DO_NOTHING 0x6
#define MASTER_MODE_UPDATE_DISTANCES 0x7
#define MASTER_MODE_ASK_EXTENSIONS 0x8
#define MASTER_MODE_AMOS 0x9
#define MASTER_MODE_ASSEMBLE_GRAPH 0xa

// slave modes
#define MODE_EXTENSION_ASK 0x0
#define MODE_START_SEEDING 0x1
#define MODE_DO_NOTHING 0x2
#define MODE_ASK_EXTENSIONS 0x3
#define MODE_SEND_EXTENSION_DATA 0x4
#define MODE_ASSEMBLE_WAVES 0x5
#define MODE_COPY_DIRECTIONS 0x6
#define MODE_ASSEMBLE_GRAPH 0x7
#define MODE_FUSION 0x8
#define MODE_MASTER_ASK_CALIBRATION 0x9
#define MODE_PERFORM_CALIBRATION 0xa
#define MODE_FINISH_FUSIONS 0xb
#define MODE_DISTRIBUTE_FUSIONS 0xc
#define MODE_AMOS 0xd
#define MODE_AUTOMATIC_DISTANCE_DETECTION 0xe
#define MODE_SEND_LIBRARY_DISTANCES 0xf
#define MODE_UPDATE_DISTANCES 0x10

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

