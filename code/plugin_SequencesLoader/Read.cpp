/*
 	Ray
    Copyright (C)  2010, 2011  Sébastien Boisvert

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
#include<application_core/common_functions.h>
#include<plugin_SequencesLoader/Read.h>
#include<cstdlib>
#include<iostream>
#include<cstring>
using namespace  std;

char*Read::trim(char*buffer,const char*sequence){
	//cout<<"In=."<<sequence<<"."<<endl;
	int theLen=strlen(sequence);
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
	while(buffer[first]!='A' && buffer[first]!='T' &&buffer[first]!='C' &&buffer[first]!='G' &&first<theLen){
		first++;
	}
	char*corrected=buffer+first;
	//cout<<"Trimmed first "<<first<<endl;
	// find the last symbol that is a A,T,C, or G
	int last=0;
	int len=strlen(corrected);
	for(int i=0;i<len;i++){
		if(corrected[i]=='A' || corrected[i]=='T' || corrected[i]=='C' || corrected[i]=='G'){
			last=i;
		}
	}
	last++;
	// only junk awaits beyond <last>
	//cout<<"Trimmed last "<<last<<endl;
	corrected[last]='\0';
	//cout<<"Out= ."<<corrected<<"."<<endl;
	//cout<<endl;
	return corrected;
}

void Read::constructorWithRawSequence(const char*seq,uint8_t*raw,bool flag){
	m_type=TYPE_SINGLE_END;
	m_length=strlen(seq);
	m_sequence=raw;
}

void Read::constructor(const char*sequence,MyAllocator*seqMyAllocator,bool trimFlag){

/*
#define DEBUG_GCC_4_7_2
#define __READ_VERBOSITY
*/

	#ifdef DEBUG_GCC_4_7_2
	cout<<"[Read::constructor] sequence is "<<sequence<<endl;
	#endif

	m_forwardOffset=0;
	m_reverseOffset=0;
	m_type=TYPE_SINGLE_END;

	char buffer[RAY_MAXIMUM_READ_LENGTH];

	if(trimFlag && strlen(sequence)<RAY_MAXIMUM_READ_LENGTH){
		sequence=trim(buffer,sequence);
	}

	#ifdef DEBUG_GCC_4_7_2
	cout<<"[DEBUG_GCC_4_7_2] after trim "<<sequence<<endl;
	#endif

	int length=strlen(sequence);
	m_length=length;

	int requiredBytes=getRequiredBytes();

	uint8_t workingBuffer[RAY_MAXIMUM_READ_LENGTH];
	for(int i=0;i<requiredBytes;i++){
		workingBuffer[i]=0;
	}

	#ifdef DEBUG_GCC_4_7_2
	cout<<"[DEBUG_GCC_4_7_2] before loop, sequence= "<<sequence<<endl;
	#endif

	for(int position=0;position<length;position++){
		char nucleotide=sequence[position];
		if(nucleotide!='A'&&nucleotide!='T'&&nucleotide!='C'&&nucleotide!='G'){

			#ifdef DEBUG_GCC_4_7_2
			cout<<"[DEBUG_GCC_4_7_2] nucleotide "<<nucleotide<<" is not in {A,T,C,G}, position "<<position<<" in "<<sequence<<", length is "<<length<<endl;
			#endif

			nucleotide='A';
		}

		uint8_t code=charToCode(nucleotide);

		#ifdef __READ_VERBOSITY
		if(position%4==0){
			cout<<"|";
		}
		cout<<" "<<(int)code<<"("<<nucleotide<<")";
		#endif

		int positionInWorkingBuffer=position/4;
		int codePositionInWord=position%4;
		uint8_t wordToUpdate=workingBuffer[positionInWorkingBuffer];
		// shift the code and or with the word to update
		code=(code<<(codePositionInWord*2));
		wordToUpdate=wordToUpdate|code;
		workingBuffer[positionInWorkingBuffer]=wordToUpdate;
	}

	#ifdef __READ_VERBOSITY
	cout<<endl;
	for(int i=0;i<requiredBytes;i++){
		cout<<" "<<(int)workingBuffer[i];
	}

	cout<<endl;
	#endif

	if(requiredBytes==0){
		m_sequence=NULL;
	}else{
		m_sequence=(uint8_t*)seqMyAllocator->allocate(requiredBytes*sizeof(uint8_t));
		memcpy(m_sequence,workingBuffer,requiredBytes);
	}
}

void Read::getSeq(char*workingBuffer,bool color,bool doubleEncoding) const{
	for(int position=0;position<m_length;position++){
		int positionInWorkingBuffer=position/4;
		uint8_t word=m_sequence[positionInWorkingBuffer];
		int codePositionInWord=position%4;
		uint8_t code=(word<<(6-codePositionInWord*2));//eliminate bits before
		code=(code>>6);
		if(!doubleEncoding)
			color=false;
		char nucleotide=codeToChar(code,color);
		workingBuffer[position]=nucleotide;
	}
	workingBuffer[m_length]='\0';
}

int Read::length()const{
	return m_length;
}

/*                      
 *           -----------------------------------
 *           -----------------------------------
 *                     p p-1 p-2               0
 */
Kmer Read::getVertex(int pos,int w,char strand,bool color) const {
	char buffer[RAY_MAXIMUM_READ_LENGTH];
	getSeq(buffer,color,false);
	return kmerAtPosition(buffer,pos,w,strand,color);
}

bool Read::hasPairedRead()const{
	return m_type!=TYPE_SINGLE_END;
}

PairedRead*Read::getPairedRead(){
	if(m_type==TYPE_SINGLE_END){
		return NULL;
	}
	return &m_pairedRead;
}

uint8_t*Read::getRawSequence(){
	return m_sequence;
}

int Read::getRequiredBytes(){
	int requiredBits=2*m_length;
	int modulo=requiredBits%8;
	if(modulo!=0){
		int bitsToAdd=8-modulo;
		requiredBits+=bitsToAdd;
	}

	#ifdef ASSERT
	assert(requiredBits%8==0);
	#endif

	int requiredBytes=requiredBits/8;
	return requiredBytes;
}

void Read::setRawSequence(uint8_t*seq,int length){
	m_sequence=seq;
	m_length=length;
}

void Read::setLeftType(){
	m_type=TYPE_LEFT_END;
}

void Read::setRightType(){
	m_type=TYPE_RIGHT_END;
}

int Read::getType(){
	return m_type;
}

void Read::setType(uint8_t a){
	m_type=a;
}

void Read::setForwardOffset(int a){
	m_forwardOffset=a;
}

void Read::setReverseOffset(int a){
	m_reverseOffset=a;
}

int Read::getForwardOffset(){
	return m_forwardOffset;
}

int Read::getReverseOffset(){
	return m_reverseOffset;
}

void Read::writeOffsets(ofstream*f){
	int forwardOffset=getForwardOffset();
	int reverseOffset=getReverseOffset();
	f->write((char*)&forwardOffset,sizeof(int));
	f->write((char*)&reverseOffset,sizeof(int));
}

void Read::readOffsets(ifstream*f){
	int forwardOffset=0;
	int reverseOffset=0;
	f->read((char*)&forwardOffset,sizeof(int));
	f->read((char*)&reverseOffset,sizeof(int));
	setForwardOffset(forwardOffset);
	setReverseOffset(reverseOffset);
}

void Read::write(ofstream*f){
	m_pairedRead.write(f);
	f->write((char*)&m_type,sizeof(uint8_t));
	f->write((char*)&m_length,sizeof(uint16_t));
	if(getRequiredBytes()>0)
		f->write((char*)m_sequence,getRequiredBytes());
}

void Read::read(ifstream*f,MyAllocator*seqMyAllocator){
	m_pairedRead.read(f);
	f->read((char*)&m_type,sizeof(uint8_t));
	f->read((char*)&m_length,sizeof(uint16_t));
	m_sequence=NULL;
	if(getRequiredBytes()>0){
		m_sequence=(uint8_t*)seqMyAllocator->allocate(getRequiredBytes()*sizeof(uint8_t));
		f->read((char*)m_sequence,getRequiredBytes());
	}
}
