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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/


#include<PairedRead.h>
#include<common_functions.h>
#include<assert.h>

PairedRead::PairedRead(){
}

void PairedRead::constructor(int rank,int id, int fragmentSize,int deviation,bool isLeftRead){
	m_isLeftRead=isLeftRead;
	m_rank=rank;
	m_readIndex=id;
	assert(fragmentSize<=MAX_U16);
	assert(deviation<=MAX_U16);
	
	m_fragmentSize=fragmentSize;
	m_deviation=deviation;
}

int PairedRead::getRank(){
	return m_rank;
}

int PairedRead::getId(){
	return m_readIndex;
}

int PairedRead::getAverageFragmentLength(){
	return m_fragmentSize;
}

int PairedRead::getStandardDeviation(){
	return m_deviation;
}

void PairedRead::updateLibrary(int d,int sd){
	// otherwise there is no need to update...
	if(m_deviation==_AUTOMATIC_DETECTION){
		m_fragmentSize=d;
		m_deviation=sd;
	}
}

u64 PairedRead::getUniqueId(){
	return m_readIndex*MAX_NUMBER_OF_MPI_PROCESSES+m_rank;
}

bool PairedRead::isLeftRead(){
	return m_isLeftRead;
}
