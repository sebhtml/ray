/*
 	OpenAssembler -- a de Bruijn DNA assembler for mixed high-throughput technologies
    Copyright (C) 2009  Sébastien Boisvert

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
#include<AvlTree.h>
#include<sys/stat.h>
#include<stdint.h>
#include<SplayTree.h>
#include<types.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<string>
#include<vector>
using namespace std;

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
string reverseComplement(string a);
VertexMer wordId(const char*a);
string idToWord(VertexMer i,int wordSize);
bool isValidDNA(const char*x);
char getLastSymbol(VertexMer i,int w);
void coutBIN(VertexMer a);
VertexMer getKPrefix(VertexMer a,int k);
VertexMer getKSuffix(VertexMer a,int k);
VertexMer complementVertex(VertexMer a,int m_wordSize);

void writePathsToFile(string file,vector<vector<VertexIndex> >*paths);

void loadPathsFromFile(string file,vector<vector<VertexIndex> >*paths);
string addLineBreaks(string sequence);

int homopolymerLength(string*s);

void loadTreeFromFile(string file,SplayTree<VertexMer,int>*tree);
void writeTreeToFile(string file,SplayTree<VertexMer,int>*tree);

void mkdir_portable(string directory);

void processSequence(const char*readSequence,int wordSize,SplayTree<VertexMer,int>*treeOfVertices,SplayTree<VertexMer,int>*treeOfEdges);

bool fileExists(string a);

void statistics(vector<vector<VertexIndex> >*paths);

uint64_t hash_uint64_t(uint64_t a);


#endif

