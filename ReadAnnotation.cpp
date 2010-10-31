/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include<ReadAnnotation.h>
#include<cstdlib>
#include<common_functions.h>

void ReadAnnotation::constructor(int rank,int readIndex,char c){
	m_uniqueId=readIndex*MAX_NUMBER_OF_MPI_PROCESSES+rank;
	m_next=NULL; // xor on the next.
	m_strand=c;
}

char ReadAnnotation::getStrand()const{
	return m_strand;
}

int ReadAnnotation::getRank()const{
	return m_uniqueId%MAX_NUMBER_OF_MPI_PROCESSES;
}

int ReadAnnotation::getReadIndex()const{
	return m_uniqueId/MAX_NUMBER_OF_MPI_PROCESSES;
}

void ReadAnnotation::setNext(ReadAnnotation*a){
	m_next=a;
}

ReadAnnotation*ReadAnnotation::getNext()const{
	return m_next;
}

int ReadAnnotation::getUniqueId()const{
	return m_uniqueId;
}

