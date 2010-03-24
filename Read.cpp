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


#include<common_functions.h>
#include<Read.h>
#include<cstdlib>
#include<iostream>
#include<cstring>

using namespace  std;

Read::Read(){
	m_sequence=NULL;
	m_pairedRead=NULL;
}


void Read::copy(const char*id,const char*sequence,MyAllocator*seqMyAllocator){
	//cout<<"IN="<<sequence<<endl;
	int theLen=strlen(sequence);
	
	char buffer[4096];
	strcpy(buffer,sequence);
	for(int i=0;i<theLen;i++){
		if(buffer[i]=='a')
			buffer[i]='A';
		else if(buffer[i]=='t')
			buffer[i]='T';
		else if(buffer[i]=='c')
			buffer[i]='C';
		else if(buffer[i]=='g')
			buffer[i]='G';
	}
	// discard N at the beginning and end of the read.
	// find the first symbol that is a A,T,C or G
	int first=0;
	while(buffer[first]!='A' and buffer[first]!='T' and buffer[first]!='C' and buffer[first]!='G' and first<theLen){
		first++;
	}
	char*corrected=buffer+first;
	// find the last symbol that is a A,T,C, or G
	int last=0;
	for(int i=0;i<(int)strlen(corrected);i++){
		if(corrected[i]=='A' or corrected[i]=='T' or corrected[i]=='C' or corrected[i]=='G'){
			last=i;
		}
	}
	last++;
	// only junk awaits beyond <last>
	corrected[last]='\0';
	//cout<<"OUT="<<corrected<<endl;
	m_sequence=(char*)seqMyAllocator->allocate(strlen(sequence)+1);
	strcpy(m_sequence,corrected); // memcpy + \0
	m_pairedRead=NULL;
}

Read::Read(const char*id,const char*sequence,MyAllocator*seqMyAllocator){
	copy(id,sequence,seqMyAllocator);
}

Read::~Read(){
}


char*Read::getSeq(){
	return m_sequence;
}


char*Read::getId(){
	return NULL;
}




int Read::length(){
	return strlen(m_sequence);
}

/*                      
 *           -----------------------------------
 *           -----------------------------------
 *                     p p-1 p-2               0
 */
VERTEX_TYPE Read::Vertex(int pos,int w,char strand,bool color){
	if(pos>length()-w){
		cout<<"Fatal: offset is too large."<<endl;
		exit(0);
	}
	if(pos<0){
		cout<<"Fatal: negative offset. "<<pos<<endl;
		exit(0);
	}
	if(strand=='F'){
		char sequence[40];
		for(int i=0;i<w;i++){
			sequence[i]=m_sequence[pos+i];
		}
		sequence[w]='\0';
		VERTEX_TYPE v=wordId(sequence);
		return v;
	}else{
		char sequence[40];
		for(int i=0;i<w;i++){
			char a=m_sequence[strlen(m_sequence)-pos-w+i];
			sequence[i]=a;
		}
		sequence[w]='\0';
		VERTEX_TYPE v=wordId(sequence);
		return complementVertex(v,w,color);
	}


	return 0;
}

void Read::setPairedRead(PairedRead*t){
	m_pairedRead=t;
}

bool Read::hasPairedRead(){
	return m_pairedRead!=NULL;
}

PairedRead*Read::getPairedRead(){
	return m_pairedRead;
}

