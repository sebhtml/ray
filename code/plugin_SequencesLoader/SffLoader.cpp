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

#include <application_core/common_functions.h>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <plugin_SequencesLoader/SffLoader.h>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

/** TODO: this code will fail on big-endian systems 
 *
 *  SFF files are stored as big-endian
 *
 *  http://en.wikipedia.org/wiki/Endianness
 *
 *  Intel processors are little-endian
 *
 *  That is why here byte order is inverted.
 * */

int max(int a,int b){
	if(a>b)
		return a;
	return b;
}

void invert16(uint16_t*c){
	uint16_t index_offset2=*c;
	char*b=(char*)&index_offset2;
	char*a=(char*)c;
	for(int i=0;i<2;i++){
		a[i]=b[2-1-i];
	}
}

void invert32(uint32_t*c){
	uint32_t index_offset2=*c;
	char*b=(char*)&index_offset2;
	char*a=(char*)c;
	for(int i=0;i<4;i++){
		a[i]=b[4-1-i];
	}
}

void invert64(uint64_t*c){
	uint64_t index_offset2=*c;
	char*b=(char*)&index_offset2;
	char*a=(char*)c;
	for(int i=0;i<8;i++){
		a[i]=b[8-1-i];
	}
}

int SffLoader::open(string file){
	m_size=0;
	m_loaded=0;
	return openSff(file);
}

int SffLoader::openSff(string file){
	uint32_t magic_number;
	uint32_t version;
	uint64_t index_offset;
	uint32_t index_length;
	uint32_t number_of_reads;
	m_fp=fopen(file.c_str(),"r");
	size_t fread_result;
	fread_result=fread((char*)&magic_number,1,sizeof(uint32_t),m_fp);
	fread_result=fread((char*)&version,1,sizeof(uint32_t),m_fp);
	invert32(&magic_number);
	invert32(&version);
	uint32_t MAGIC=0x2e736666;
	uint32_t _VERSION=1;

	if(MAGIC!=magic_number){
		(cout)<<"Error: incorrect magic number "<<endl;
		printf("%x\n",magic_number);
		printf("%x\n",MAGIC);
		return EXIT_FAILURE;
	}

	if(_VERSION!=version){
		(cout)<<"Error: incorrect version"<<endl;
		return EXIT_FAILURE;
	}

	fread_result=fread((char*)&index_offset,1,sizeof(uint64_t),m_fp);
	invert64(&index_offset);
	fread_result=fread((char*)&index_length,1,sizeof(uint32_t),m_fp);
	fread_result=fread((char*)&number_of_reads,1,sizeof(uint32_t),m_fp);
	invert32(&index_length);
	invert32(&number_of_reads);
	
	m_size=number_of_reads;

	uint16_t header_length;
	fread_result=fread((char*)&header_length,1,sizeof(uint16_t),m_fp);
	invert16(&header_length);
	
	fread_result=fread((char*)&key_length,1,sizeof(uint16_t),m_fp);
	invert16(&key_length);
	fread_result=fread((char*)&m_number_of_flows_per_read,1,sizeof(uint16_t),m_fp);
	invert16(&m_number_of_flows_per_read);
	uint8_t flowgram_format_code;
	fread_result=fread((char*)&flowgram_format_code,1,sizeof(uint8_t),m_fp);
	flow_chars=(char*)__Malloc(m_number_of_flows_per_read+1,"RAY_MALLOC_TYPE_454",false);
	fread_result=fread(flow_chars,1,m_number_of_flows_per_read,m_fp);
	flow_chars[m_number_of_flows_per_read]='\0';
	key_sequence=(char*)__Malloc(key_length+1,"RAY_MALLOC_TYPE_454",false);
	fread_result=fread(key_sequence,1,key_length,m_fp);
	key_sequence[key_length]='\0';
	
	if(fread_result)
		fread_result=0;

	// padding
	while(ftell(m_fp)%8!=0){
		fgetc(m_fp);
	}
	return EXIT_SUCCESS;
}

// see manual http://sequence.otago.ac.nz/download/GS_FLX_Software_Manual.pdf,
// page 445-448
// or 
// http://blog.malde.org/index.php/2008/11/14/454-sequencing-and-parsing-the-sff-binary-format/
void SffLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){

	int loadedSequences=0;
	size_t fread_result;

	while(m_loaded<m_size && loadedSequences<maxToLoad){
		uint16_t read_header_length;
		fread_result=fread((char*)&read_header_length,1,sizeof(uint16_t),m_fp);
		invert16(&read_header_length);
		uint16_t name_length;
		fread_result=fread((char*)&name_length,1,sizeof(uint16_t),m_fp);
		invert16(&name_length);
		uint32_t number_of_bases;
		fread_result=fread((char*)&number_of_bases,1,sizeof(uint32_t),m_fp);
		invert32(&number_of_bases);
		uint16_t clip_qual_left;
		fread_result=fread((char*)&clip_qual_left,1,sizeof(uint16_t),m_fp);
		invert16(&clip_qual_left);
		uint16_t clip_qual_right;
		fread_result=fread((char*)&clip_qual_right,1,sizeof(uint16_t),m_fp);
		invert16(&clip_qual_right);
		uint16_t clip_adaptor_left;
		fread_result=fread((char*)&clip_adaptor_left,1,sizeof(uint16_t),m_fp);
		invert16(&clip_adaptor_left);
		uint16_t clip_adaptor_right;
		fread_result=fread((char*)&clip_adaptor_right,1,sizeof(uint16_t),m_fp);
		invert16(&clip_adaptor_right);
		char*Name=(char*)__Malloc(name_length+1,"RAY_MALLOC_TYPE_454",false);
		fread_result=fread(Name,1,name_length,m_fp);
		Name[name_length]='\0';

		// padding
		while(ftell(m_fp)%8!=0)
			fgetc(m_fp);

		int skip=m_number_of_flows_per_read*sizeof(uint16_t);
		for(int i=0;i<skip;i++)
			fgetc(m_fp);
		skip=number_of_bases*sizeof(uint8_t);
		for(int i=0;i<skip;i++)
			fgetc(m_fp);
		char*Bases=(char*)__Malloc(number_of_bases+1,"RAY_MALLOC_TYPE_454",false);
		fread_result=fread(Bases,1,number_of_bases,m_fp);
		Bases[number_of_bases]='\0';
		skip=number_of_bases*sizeof(uint8_t);
		for(int i=0;i<skip;i++)
			fgetc(m_fp);

		// padding
		while(ftell(m_fp)%8!=0)
			fgetc(m_fp);

		int first=max(1,max(clip_qual_left,clip_adaptor_left));
		int last=min((clip_qual_right==0?number_of_bases:clip_qual_right),
				(clip_adaptor_right==0?number_of_bases:clip_adaptor_right));

		string sequence=Bases;
		string key=key_sequence;

		Read read;
		read.constructor(sequence.substr(first-1,last-first+1).c_str(),seqMyAllocator,true);
		reads->push_back(&read);
	
		loadedSequences++;
		m_loaded++;

		__Free(Name,"RAY_MALLOC_TYPE_454",false);
		__Free(Bases,"RAY_MALLOC_TYPE_454",false);
	}

	if(fread_result)
		fread_result=0;

	if(m_loaded==m_size){
		__Free(key_sequence,"RAY_MALLOC_TYPE_454",false);
		__Free(flow_chars,"RAY_MALLOC_TYPE_454",false);
		fclose(m_fp);
	}
}

int SffLoader::getSize(){
	return m_size;
}
