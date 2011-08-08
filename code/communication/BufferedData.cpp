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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#include <assert.h>
#include <communication/BufferedData.h>
#include <memory/RingAllocator.h>
#include <stdio.h>
#include <stdlib.h>
#include <structures/StaticVector.h>
#include <iostream>
using namespace std;

// the capacity is measured in uint64_t
void BufferedData::constructor(int numberOfRanks,int capacity,int type,bool show,int period){
	m_period=period;
	m_flushedMessages=0;
	m_count=0;
	m_pushedMessages=0;
	m_show=show;
	m_type=type;
	#ifdef DEBUG_BUFFERS
	printf("BufferedData::constructor\n");
	fflush(stdout);
	#endif

	m_sizes=(int*)__Malloc(sizeof(int)*numberOfRanks,m_type,m_show);
	m_data=(uint64_t*)__Malloc(sizeof(uint64_t)*capacity*numberOfRanks,m_type,m_show);
	for(int i=0;i<(int)numberOfRanks;i++){
		m_sizes[i]=0;
	}
	m_capacity=capacity;
	m_ranks=numberOfRanks;
}

void BufferedData::clear(){
	#ifdef ASSERT
	assert(isEmpty());
	#endif

	if(m_sizes!=NULL){
		__Free(m_sizes,m_type,m_show);
		m_sizes=NULL;
	}
	if(m_data!=NULL){
		__Free(m_data,m_type,m_show);
		m_data=NULL;
	}
}

int BufferedData::size(int i)const{
	#ifdef ASSERT
	if(i>=m_ranks){
		cout<<"i="<<i<<" m_ranks="<<m_ranks<<endl;
	}
	assert(i<m_ranks);
	#endif
	return m_sizes[i];
}

uint64_t BufferedData::getAt(int i,int j){
	return m_data[i*m_capacity+j];
}

void BufferedData::addAt(int i,uint64_t k){
	int j=size(i);
	m_data[i*m_capacity+j]=k;
	m_sizes[i]++;
	m_count++;
	if(m_count==m_period){
		m_count=0;
		m_pushedMessages++;
	}
}

void BufferedData::reset(int i){
	m_sizes[i]=0;
	#ifdef ASSERT
	assert(m_sizes[i]==0);
	#endif
}

bool BufferedData::isEmpty()const{
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
		threshold=(m_capacity/period)*period;
	}

	#ifdef ASSERT
	assert(threshold<=m_capacity);
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
	if(!force && amount>threshold){
		cout<<"Threshold is exceeded "<<amount<<" & "<<threshold<<" tag="<<tag<<endl;
	}
	assert(force || amount<=threshold);
	#endif

	uint64_t*message=(uint64_t*)outboxAllocator->allocate(amount*sizeof(uint64_t));
	for(int i=0;i<amount;i++){
		message[i]=getAt(destination,i);
	}
	Message aMessage(message,amount,destination,tag,rank);
	outbox->push_back(aMessage);
	m_flushedMessages++;
	reset(destination);
	return true;
}

bool BufferedData::needsFlushing(int destination,int period){
	int threshold=(m_capacity/period)*period;
	int amount=size(destination);
	return amount>=threshold;
}

void BufferedData::showStatistics(int rank){
	double ratio=100.0*m_flushedMessages/m_pushedMessages;
	printf("Rank %i: VirtualCommunicator: %lu pushed messages generated %lu virtual messages (%f%%)\n",rank,m_pushedMessages,m_flushedMessages,ratio);
	fflush(stdout);
}
