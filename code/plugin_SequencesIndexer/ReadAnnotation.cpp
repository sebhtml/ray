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

#include<assert.h>
#include<plugin_SequencesIndexer/ReadAnnotation.h>
#include<stdlib.h>
#include<application_core/common_functions.h>

void ReadAnnotation::constructor(int rank,int readIndex,int positionOnStrand,char c,bool lower){
	m_lower=lower;
	m_rank=rank;
	m_positionOnStrand=positionOnStrand;
	m_readIndex=readIndex;
	m_next=NULL; // xor on the next.
	m_strand=c;
}

int ReadAnnotation::getPositionOnStrand()const{
	return m_positionOnStrand;
}

char ReadAnnotation::getStrand()const{
	return m_strand;
}

int ReadAnnotation::getRank()const{
	return m_rank;
}

int ReadAnnotation::getReadIndex()const{
	return m_readIndex;
}

void ReadAnnotation::setNext(ReadAnnotation*a){
	m_next=a;
}

ReadAnnotation*ReadAnnotation::getNext()const{
	return m_next;
}

ReadHandle ReadAnnotation::getUniqueId()const{
	return getPathUniqueId(m_rank,m_readIndex);
}

bool ReadAnnotation::isLower(){
	return m_lower;
}

void ReadAnnotation::write(ofstream*f){
	int rank=getRank();
	int readIndex=getReadIndex();
	int positionOnStrand=getPositionOnStrand();
	char strand=getStrand();
	f->write((char*)&rank,sizeof(int));
	f->write((char*)&readIndex,sizeof(int));
	f->write((char*)&positionOnStrand,sizeof(int));
	f->write((char*)&strand,sizeof(char));
}

void ReadAnnotation::read(ifstream*f,bool isLower){
	int rank=0;
	int readIndex=0;
	int positionOnStrand=0;
	char strand=0;
	f->read((char*)&rank,sizeof(int));
	f->read((char*)&readIndex,sizeof(int));
	f->read((char*)&positionOnStrand,sizeof(int));
	f->read((char*)&strand,sizeof(char));
	constructor(rank,readIndex,positionOnStrand,strand,isLower);
}
