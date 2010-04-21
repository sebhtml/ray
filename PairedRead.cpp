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


#include<PairedRead.h>

PairedRead::PairedRead(){
}

void PairedRead::constructor(int rank,int id, int fragmentSize,int deviation){
	m_rank=rank;
	m_sequence_id=id;
	m_fragmentSize=fragmentSize;
	m_deviation=deviation;
	#ifdef DEBUG_PARAMETERS
	cout<<"PairedRead: "<<m_fragmentSize<<","<<m_deviation<<endl;
	#endif
}


int PairedRead::getRank(){
	return m_rank;
}

int PairedRead::getId(){
	return m_sequence_id;
}

int PairedRead::getAverageFragmentLength(){
	return m_fragmentSize;
}

int PairedRead::getStandardDeviation(){
	return m_deviation;
}

void PairedRead::updateLibrary(int d,int sd){
	m_fragmentSize=d;
	m_deviation=sd;
}
