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


#ifndef _Read
#define _Read

#include<string>
#include<stdint.h>
#include<vector>
#include<common_functions.h>
#include<MyAllocator.h>
#include<PairedRead.h>
using namespace std;

/*
 * a read is represented as a char*
 * and a (possible) link to paired information.
 */
class Read{
	char*m_sequence;
	PairedRead*m_pairedRead;// the read on the left
	char*trim(char*a,const char*b);
public:
	Read();
	Read(const char*id,const char*sequence,MyAllocator*seqMyAllocator);
	~Read();
	char*getSeq();
	int length();
	VERTEX_TYPE Vertex(int pos,int w,char strand,bool color);
	void copy(const char*id,const char*sequence,MyAllocator*seqMyAllocator);
	void setPairedRead(PairedRead*t);
	bool hasPairedRead();
	PairedRead*getPairedRead();
};

#endif
