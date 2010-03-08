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


#include<cstring>
#include<fstream>
#include<string>
#include<vector>
#include<SffLoader.h>
#include<iostream>
#include<stdint.h>
#include<stdio.h>
using namespace std;

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

// see manual http://sequence.otago.ac.nz/download/GS_FLX_Software_Manual.pdf,
// page 445-448
// or 
// http://blog.malde.org/index.php/2008/11/14/454-sequencing-and-parsing-the-sff-binary-format/
void SffLoader::load(string file,vector<Read*>*reads,MyAllocator*seqMyAllocator,MyAllocator*readMyAllocator){
	(cout)<<"[SffLoader::load]"<<endl;
	uint32_t magic_number;
	uint32_t version;
	uint64_t index_offset;
	uint32_t index_length;
	uint32_t number_of_reads;
	FILE*fp=fopen(file.c_str(),"r");
	size_t fread_result;
	fread_result=fread((char*)&magic_number,1,sizeof(uint32_t),fp);
	fread_result=fread((char*)&version,1,sizeof(uint32_t),fp);
	invert32(&magic_number);
	invert32(&version);
	uint32_t MAGIC=0x2e736666;
	uint32_t _VERSION=1;
	if(MAGIC!=magic_number){
		(cout)<<"Error: incorrect magic number "<<endl;
		printf("%x\n",magic_number);
		printf("%x\n",MAGIC);
		return;
	}
	if(_VERSION!=version){
		(cout)<<"Error: incorrect version"<<endl;
		return;
	}
	fread_result=fread((char*)&index_offset,1,sizeof(uint64_t),fp);
	invert64(&index_offset);
	(cout)<<"Using clip values"<<endl;
	(cout)<<"Index offset: "<<index_offset<<endl;
	fread_result=fread((char*)&index_length,1,sizeof(uint32_t),fp);
	fread_result=fread((char*)&number_of_reads,1,sizeof(uint32_t),fp);
	invert32(&index_length);
	(cout)<<"Index length: "<<index_length<<endl;
	invert32(&number_of_reads);
	(cout)<<"Reads: "<<number_of_reads<<endl;
	uint16_t header_length;
	fread_result=fread((char*)&header_length,1,sizeof(uint16_t),fp);
	invert16(&header_length);
	(cout)<<"Header: "<<header_length<<endl;
	uint16_t key_length;
	
	fread_result=fread((char*)&key_length,1,sizeof(uint16_t),fp);
	invert16(&key_length);
	(cout)<<"Key Length: "<<(int)key_length<<endl;
	uint16_t number_of_flows_per_read;
	fread_result=fread((char*)&number_of_flows_per_read,1,sizeof(uint16_t),fp);
	invert16(&number_of_flows_per_read);
	uint8_t flowgram_format_code;
	fread_result=fread((char*)&flowgram_format_code,1,sizeof(uint8_t),fp);
	(cout)<<"number_of_flows_per_read: "<<number_of_flows_per_read<<endl;
	char*flow_chars=new char[number_of_flows_per_read+1];
	fread_result=fread(flow_chars,1,number_of_flows_per_read,fp);
	flow_chars[number_of_flows_per_read]='\0';
	char*key_sequence=new char[key_length+1];
	fread_result=fread(key_sequence,1,key_length,fp);
	key_sequence[key_length]='\0';
	
	(cout)<<"key: "<<key_sequence<<endl;
	
	// padding
	while(ftell(fp)%8!=0)
		fgetc(fp);

	for(int readId=0;readId<(int)number_of_reads;readId++){
		uint16_t read_header_length;
		fread_result=fread((char*)&read_header_length,1,sizeof(uint16_t),fp);
		invert16(&read_header_length);
		uint16_t name_length;
		fread_result=fread((char*)&name_length,1,sizeof(uint16_t),fp);
		invert16(&name_length);
		uint32_t number_of_bases;
		fread_result=fread((char*)&number_of_bases,1,sizeof(uint32_t),fp);
		invert32(&number_of_bases);
		uint16_t clip_qual_left;
		fread_result=fread((char*)&clip_qual_left,1,sizeof(uint16_t),fp);
		invert16(&clip_qual_left);
		uint16_t clip_qual_right;
		fread_result=fread((char*)&clip_qual_right,1,sizeof(uint16_t),fp);
		invert16(&clip_qual_right);
		uint16_t clip_adaptor_left;
		fread_result=fread((char*)&clip_adaptor_left,1,sizeof(uint16_t),fp);
		invert16(&clip_adaptor_left);
		uint16_t clip_adaptor_right;
		fread_result=fread((char*)&clip_adaptor_right,1,sizeof(uint16_t),fp);
		invert16(&clip_adaptor_right);
		char*Name=new char[name_length+1];
		fread_result=fread(Name,1,name_length,fp);
		Name[name_length]='\0';

		// padding
		while(ftell(fp)%8!=0)
			fgetc(fp);

		int skip=number_of_flows_per_read*sizeof(uint16_t);
		for(int i=0;i<skip;i++)
			fgetc(fp);
		skip=number_of_bases*sizeof(uint8_t);
		for(int i=0;i<skip;i++)
			fgetc(fp);
		char*Bases=new char[number_of_bases+1];
		fread_result=fread(Bases,1,number_of_bases,fp);
		Bases[number_of_bases]='\0';
		skip=number_of_bases*sizeof(uint8_t);
		for(int i=0;i<skip;i++)
			fgetc(fp);

		// padding
		while(ftell(fp)%8!=0)
			fgetc(fp);

		int first=max(1,max(clip_qual_left,clip_adaptor_left));
		int last=min((clip_qual_right==0?number_of_bases:clip_qual_right),
				(clip_adaptor_right==0?number_of_bases:clip_adaptor_right));

		string sequence=Bases;
		string key=key_sequence;
		if(sequence.substr(0,key_length)!=key){
			(cout)<<"Not KEY, was "<<sequence.substr(0,key_length)<<" expected "<<key<<endl;
			continue;
		}
		Read*read=(Read*)readMyAllocator->allocate(sizeof(Read));
		read->copy(Name,sequence.substr(first-1,last-first+1).c_str(),seqMyAllocator);
		reads->push_back(read);
		m_bases+=strlen(read->getSeq());
		delete[]Name;
		delete[]Bases;
	}

	delete[] key_sequence;
	delete[] flow_chars;
	fclose(fp);
}

SffLoader::SffLoader(){
	m_bases=0;
}

int SffLoader::getBases(){
	return m_bases;
}
