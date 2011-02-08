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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

// TYPE: FRAMEWORK

#ifndef _ReadAnnotation
#define _ReadAnnotation

#include<common_functions.h>

/*
 * implemented as linked lists, read annotations give 
 * information about read index in the graph.
 * The read paths are restored using read annotations.
 */
class ReadAnnotation{
	ReadAnnotation*m_next;
	uint32_t m_readIndex;
	uint16_t m_rank;
	char m_strand;
	bool m_lower;
public:
	void constructor(int a,int b,char c,bool lower);
	bool isLower();
	int getRank()const;
	int getReadIndex()const;
	char getStrand()const;
	ReadAnnotation*getNext()const;
	void setNext(ReadAnnotation*a);
	uint64_t getUniqueId() const;
}
#ifdef __GNUC__ 
__attribute((packed)) 
#endif
;

#endif

