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

#include <structures/Kmer.h>
#include <stdio.h>
#include <assert.h>

bool Kmer::operator<(const Kmer&b)const{
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		if(m_u64[i]<b.m_u64[i]){
			return true;
		}else if(m_u64[i]>b.m_u64[i]){
			return false;
		}
	}
	return false;
}

Kmer::Kmer(){
	for(int i=0;i<getNumberOfU64();i++){
		setU64(i,0);
	}
}

Kmer::~Kmer(){
}

int Kmer::getNumberOfU64(){
	return KMER_U64_ARRAY_SIZE;
}

bool Kmer::isLower(Kmer*a){
	for(int i=0;i<getNumberOfU64();i++){
		if(getU64(i)<a->getU64(i)){
			return true;
		}else if(getU64(i)>a->getU64(i)){
			return false;
		}
	}
	return false;
}

bool Kmer::isEqual(Kmer*a){
	for(int i=0;i<getNumberOfU64();i++){
		if(getU64(i)!=a->getU64(i)){
			return false;
		}
	}
	return true;
}

void Kmer::print(){
	for(int j=0;j<getNumberOfU64();j++){
		uint64_t a=getU64(j);
		for(int k=63;k>=0;k-=2){
			int bit=a<<(k-1)>>63;
			printf("%i",bit);
			bit=a<<(k)>>63;
			printf("%i ",bit);
		}
	}
	printf("\n");
}

void Kmer::pack(uint64_t*messageBuffer,int*messagePosition){
	for(int i=0;i<getNumberOfU64();i++){
		messageBuffer[*messagePosition]=getU64(i);
		(*messagePosition)++;
	}
}

void Kmer::unpack(uint64_t*messageBuffer,int*messagePosition){
	for(int i=0;i<getNumberOfU64();i++){
		setU64(i,messageBuffer[*messagePosition]);
		(*messagePosition)++;
	}
}

void Kmer::unpack(vector<uint64_t>*messageBuffer,int*messagePosition){
	for(int i=0;i<getNumberOfU64();i++){
		setU64(i,(*messageBuffer)[*messagePosition]);
		(*messagePosition)++;
	}
}

void Kmer::operator=(const Kmer&b){
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		m_u64[i]=b.m_u64[i];
	}
}

bool Kmer::operator==(const Kmer&b) const{
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		if(m_u64[i]!=b.m_u64[i]){
			return false;
		}
	}
	return true;
}

bool Kmer::operator!=(const Kmer&b) const{
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		if(m_u64[i]!=b.m_u64[i]){
			return true;
		}
	}
	return false;
}

