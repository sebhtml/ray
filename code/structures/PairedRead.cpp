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

#include<structures/PairedRead.h>
#include<core/common_functions.h>
#include<assert.h>

void PairedRead::constructor(int rank,int id,int library){
	m_rank=rank;
	m_readIndex=id;
	m_library=library;
}

int PairedRead::getRank(){
	return m_rank;
}

uint32_t PairedRead::getId(){
	return m_readIndex;
}

/** any read has a unique distributed identifier */
uint64_t PairedRead::getUniqueId(){
	return getPathUniqueId(m_rank,m_readIndex);
}

int PairedRead::getLibrary(){
	return m_library;
}
