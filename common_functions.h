/*
 	Ray
    Copyright (C) 2009, 2010  Sébastien Boisvert

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


#define MAX_NUMBER_OF_MPI_PROCESSES 10000

uint64_t wordId(const char*a);
string idToWord(uint64_t i,int wordSize);
bool isValidDNA(const char*x);
char getFirstSymbol(uint64_t i,int k);
char getLastSymbol(uint64_t i,int w);
void coutBIN(uint64_t a);
uint64_t getKPrefix(uint64_t a,int k);
uint64_t getKSuffix(uint64_t a,int k);
uint64_t complementVertex(uint64_t a,int m_wordSize);


string addLineBreaks(string sequence);

uint64_t hash_uint64_t(uint64_t a);


#endif

