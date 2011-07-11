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

#include <communication/NetworkTest.h>
#include <communication/Message.h>
#include <communication/mpi_tags.h>
#include <core/slave_modes.h>
#include <core/master_modes.h>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <core/constants.h>
#include <iostream>
using namespace std;

#define LATENCY_INFORMATION_NOT_AVAILABLE 123123123

/** initialize the NetworkTest */
void NetworkTest::constructor(int rank,int*masterMode,int*slaveMode,int size,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,RingAllocator*outboxAllocator,
	string*name){
	m_name=name;
	m_inbox=inbox;
	m_outbox=outbox;
	m_parameters=parameters;
	m_initialisedNetworkTest=false;
	m_size=size;
	m_rank=rank;
	m_masterMode=masterMode;
	m_slaveMode=slaveMode;
	m_numberOfTestMessages=100000;
	m_currentTestMessage=0;
	m_sentCurrentTestMessage=false;
	m_outboxAllocator=outboxAllocator;
	m_sumOfMicroSeconds=0;
	srand(time(NULL)*(1+m_rank));
}

/** call the slave method 
 *
 * To test the network:
 * 
 * n=1000
 *
 * i=n
 * while i--
 *     destination <- random folk
 *     send full message to destination
 *     start timer
 *     receive message
 *     stop timer
 *     diff=stop-start
 *     sum+=diff
 *
 * average latency <- sum / n
 *
 * if rank==0
 *
 *   write this information to a file.
 *
 * */
void NetworkTest::slaveWork(){
	if(m_currentTestMessage<m_numberOfTestMessages){
		if(!m_sentCurrentTestMessage){
			#ifdef OS_POSIX
			gettimeofday(&m_startingTime,NULL);
			#endif
			/** send to a random rank */
			int destination=rand()%m_size;
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
			Message aMessage(message,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),destination,RAY_MPI_TAG_TEST_NETWORK_MESSAGE,m_rank);
			m_outbox->push_back(aMessage);
			m_sentCurrentTestMessage=true;
			//cout<<m_rank<<" sends RAY_MPI_TAG_TEST_NETWORK_MESSAGE to "<<destination<<endl;
		}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()==RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY){
			#ifdef OS_POSIX
			struct timeval endingTime;
			gettimeofday(&endingTime,NULL);

			int startingSeconds=m_startingTime.tv_sec;
			int startingMicroSeconds=m_startingTime.tv_usec;
			int endingSeconds=endingTime.tv_sec;
			int endingMicroSeconds=endingTime.tv_usec;
			
			int microSeconds=(endingSeconds-startingSeconds)*1000*1000+endingMicroSeconds-startingMicroSeconds;
			m_sumOfMicroSeconds+=microSeconds;

			#endif
			m_sentCurrentTestMessage=false;
			m_currentTestMessage++;
		}
	}else{
		int averageLatencyInMicroSeconds=LATENCY_INFORMATION_NOT_AVAILABLE;

		#ifdef OS_POSIX
		averageLatencyInMicroSeconds=m_sumOfMicroSeconds/m_numberOfTestMessages;
		#endif

		cout<<"Rank "<<m_rank<<": average latency for "<<(*m_name)<<" when requesting a reply for a message of "<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<" bytes is "<<averageLatencyInMicroSeconds<<" microseconds (10^-6 seconds)"<<endl;

		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		message[0]=averageLatencyInMicroSeconds;
		char*destination=(char*)(message+1);
		strcpy(destination,m_name->c_str());
		Message aMessage(message,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),MASTER_RANK,RAY_MPI_TAG_TEST_NETWORK_REPLY,m_rank);
		m_outbox->push_back(aMessage);
		(*m_slaveMode)=RAY_SLAVE_MODE_DO_NOTHING;
	}
}

/** call the master method */
void NetworkTest::masterWork(){
	if(!m_initialisedNetworkTest){
		cout<<"Rank 0: testing the network, please wait..."<<endl;
		cout<<endl;
		for(int i=0;i<m_size;i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_TEST_NETWORK,m_rank);
			m_outbox->push_back(aMessage);
		}
		m_doneWithNetworkTest=0;
		m_initialisedNetworkTest=true;
	}else if(m_inbox->size()>0&&(*m_inbox)[0]->getTag()==RAY_MPI_TAG_TEST_NETWORK_REPLY){
		int rank=m_inbox->at(0)->getSource();
		int latency=m_inbox->at(0)->getBuffer()[0];
		uint64_t*buffer=m_inbox->at(0)->getBuffer();
		char*name=(char*)(buffer+1);
		string stringName=name;
		m_names[rank]=stringName;
		m_latencies[rank]=latency;
		m_doneWithNetworkTest++;
	}else if(m_doneWithNetworkTest==m_size){
		ostringstream file;
		file<<m_parameters->getPrefix();
		file<<".NetworkTest.txt";
		ofstream f(file.str().c_str());
		f<<"# average latency in microseconds (10^-6 seconds) when requesting a reply for a message of "<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<" bytes"<<endl;
		f<<"# Message passing interface rank\tName\tLatency in microseconds"<<endl;
		for(int i=0;i<m_size;i++){
			int latency=m_latencies[i];
			if(latency==LATENCY_INFORMATION_NOT_AVAILABLE)
				f<<i<<"\t"<<m_names[i]<<"\tLATENCY_INFORMATION_NOT_AVAILABLE"<<endl;
			else
				f<<i<<"\t"<<m_names[i]<<"\t"<<latency<<endl;
		}
		f.close();
		m_latencies.clear();
		(*m_masterMode)=RAY_MASTER_MODE_LOAD_SEQUENCES;
	}
}
