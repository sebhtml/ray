/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#ifndef _KmerAcademyIterator
#define _KmerAcademyIterator

#include <core/Parameters.h>
#include <core/common_functions.h>
#include <structures/MyHashTableIterator.h>
#include <graph/KmerAcademy.h>

class KmerAcademyIterator{
	MyHashTableIterator<Kmer,KmerCandidate> m_iterator;
	bool m_mustProcessOtherKey;
	Kmer m_currentKey;
	KmerCandidate*m_currentEntry;
	Parameters*m_parameters;
public:
	void constructor(KmerAcademy*a,int wordSize,Parameters*b);
	bool hasNext();
	KmerCandidate*next();
	Kmer*getKey();
};

#endif
