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

#include<assert.h>
#include<mpi.h>
#include<BufferedData.h>
#include<RingAllocator.h>
#include<StaticVector.h>

void BufferedData::constructor(int numberOfRanks,int capacity){
	m_sizes=(int*)__Malloc(sizeof(int)*numberOfRanks);
	m_data=(u64*)__Malloc(sizeof(u64)*capacity*numberOfRanks);
	for(int i=0;i<(int)numberOfRanks;i++){
		m_sizes[i]=0;
	}
	m_capacity=capacity;
	m_ranks=numberOfRanks;
}

void BufferedData::clear(){
	if(m_sizes!=NULL){
		__Free(m_sizes);
		m_sizes=NULL;
	}
	if(m_data!=NULL){
		__Free(m_data);
		m_data=NULL;
	}
}

int BufferedData::size(int i){
	#ifdef ASSERT
	assert(i<m_ranks);
	#endif
	return m_sizes[i];
}

u64 BufferedData::getAt(int i,int j){
	return m_data[i*m_capacity+j];
}

void BufferedData::addAt(int i,u64 k){
	int j=size(i);
	m_data[i*m_capacity+j]=k;
	m_sizes[i]++;
}

void BufferedData::reset(int i){
	m_sizes[i]=0;
	#ifdef ASSERT
	assert(m_sizes[i]==0);
	#endif
}

bool BufferedData::isEmpty(){
	if(m_sizes==NULL&&m_data==NULL){
		return true;
	}

	for(int i=0;i<m_ranks;i++){
		if(size(i)!=0){
			return false;
		}
	}
	return true;
}

int BufferedData::flushAll(int tag,RingAllocator*outboxAllocator,StaticVector*outbox,int rank){
	if(isEmpty()){
		return 0;
	}
	int flushed=0;
	for(int i=0;i<m_ranks;i++){
		if(flush(i,0,tag,outboxAllocator,outbox,rank,true)){
			flushed++;
			return flushed;
		}
		#ifdef ASSERT
		assert(size(i)==0);
		#endif
	}
	return flushed;
}



bool BufferedData::flush(int destination,int period,int tag,RingAllocator*outboxAllocator,StaticVector*outbox,int rank,bool force){
	#ifdef ASSERT
	if(!force){
		assert(period!=0);
	}
	#endif

	int threshold=0;
	if(!force){
		threshold=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)/period*period;
	}

	#ifdef ASSERT
	assert(threshold<MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif

	int amount=size(destination);
	if(!force && amount<threshold){
		return false;
	}
	if(amount==0){
		return false;
	}
	#ifdef ASSERT
	assert(amount>0);
	#endif
	uint64_t*message=(uint64_t*)outboxAllocator->allocate(amount*sizeof(uint64_t));
	for(int i=0;i<amount;i++){
		message[i]=getAt(destination,i);
	}
	Message aMessage(message,amount,MPI_UNSIGNED_LONG_LONG,destination,tag,rank);
	outbox->push_back(aMessage);
	reset(destination);
	return true;
}

bool BufferedData::needsFlushing(int destination,int period){
	int threshold=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)/period*period;
	int amount=size(destination);
	return amount>=threshold;
}
