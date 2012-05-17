/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#ifndef _ReadAnnotation
#define _ReadAnnotation

#include <application_core/common_functions.h>
#include <fstream>
using namespace std;

/*
 * implemented as linked lists, read annotations give 
 * information about read index in the graph.
 * The read paths are restored using read annotations.
 * \author Sébastien Boisvert
 */
class ReadAnnotation{
	ReadAnnotation*m_next;
	uint32_t m_readIndex;
	uint16_t m_rank; // should be Rank
	uint16_t m_positionOnStrand;
	Strand m_strand;
	bool m_lower;
public:
	void constructor(int a,int b,int positionOnStrand,char c,bool lower);
	bool isLower();
	int getRank()const;
	int getReadIndex()const;
	int getPositionOnStrand()const;
	Strand getStrand()const;
	ReadAnnotation*getNext()const;
	void setNext(ReadAnnotation*a);
	ReadHandle getUniqueId() const;

	void read(ifstream*f,bool isLower);
	void write(ofstream*f);
} ATTRIBUTE_PACKED;

#endif

