/*
 	Ray
    Copyright (C) 2011, 2012  Sébastien Boisvert

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

#include <communication/NetworkTest.h>
#include <communication/Message.h>
#include <communication/mpi_tags.h>
#include <core/slave_modes.h>
#include <core/master_modes.h>
#include <sstream>
#include <core/OperatingSystem.h>
#include <fstream>
#include <stdlib.h>
#include <core/statistics.h>
#include <core/constants.h>
#include <iostream>
#include <core/ComputeCore.h>
using namespace std;

#define LATENCY_INFORMATION_NOT_AVAILABLE 123123123

/** initialize the NetworkTest */
void NetworkTest::constructor(int rank,int size,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,RingAllocator*outboxAllocator,
	string*name,TimePrinter*timePrinter){
	m_timePrinter=timePrinter;
	m_name=name;
	m_inbox=inbox;
	m_outbox=outbox;
	m_parameters=parameters;
	m_initialisedNetworkTest=false;
	m_size=size;
	m_rank=rank;
	
	m_ranksFinished=0;
	m_askedToWriteFiles=false;

	int ranksPerNode=8;
	int onlineRanksPerNode=8; // default: 8

	if((m_rank % ranksPerNode) >= onlineRanksPerNode)
		m_numberOfTestMessages=0;

	m_currentTestMessage=0;
	m_sentCurrentTestMessage=false;
	m_outboxAllocator=outboxAllocator;

	/* a word is 8 bytes */
	/* MAXIMUM_MESSAGE_SIZE_IN_BYTES is 4000 per default so 
		numberOfWords must be <= 500 */
	/* this is only for the network test */
	/* default is 500 */
	m_numberOfWords=500;

	m_numberOfTestMessages=m_parameters->getSize()*1000;

	m_writeRawData=m_parameters->hasOption("-write-network-test-raw-data");
	/* the seed must be different for all MPI ranks */
	srand(time(NULL)*(1+m_rank));

	// reserve space to have constant insertion
	m_sentMicroseconds.reserve(m_numberOfTestMessages);
	m_destinations.reserve(m_numberOfTestMessages);
	m_receivedMicroseconds.reserve(m_numberOfTestMessages);
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
 *
 * if rank==0
 *
 *   write this information to a file.
 *
 * */
void NetworkTest::call_RAY_SLAVE_MODE_TEST_NETWORK(){


	#ifdef ASSERT
	assert(m_numberOfWords*sizeof(uint64_t) <= MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif

	if(m_currentTestMessage<m_numberOfTestMessages){
		if(!m_sentCurrentTestMessage){

			if(m_currentTestMessage==0)
				m_sentData=false;

			uint64_t startingTimeMicroseconds=getMicroseconds();

			/** send to a random rank */
			int destination=rand()%m_size;

			m_sentMicroseconds.push_back(startingTimeMicroseconds);
			m_destinations.push_back(destination);

			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(m_numberOfWords*sizeof(uint64_t));
			Message aMessage(message,m_numberOfWords,destination,RAY_MPI_TAG_TEST_NETWORK_MESSAGE,m_rank);
			m_outbox->push_back(aMessage);
			m_sentCurrentTestMessage=true;
			//cout<<m_rank<<" sends RAY_MPI_TAG_TEST_NETWORK_MESSAGE to "<<destination<<endl;
		}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()==RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY){
			uint64_t endingMicroSeconds=getMicroseconds();
			
			m_receivedMicroseconds.push_back(endingMicroSeconds);

			m_sentCurrentTestMessage=false;
			m_currentTestMessage++;
		}
	}else if(!m_sentData){
		// we finished gathering data.
		// now we compute the mode for the latency
		// TODO: this should probably done after everyone has finished

		int latency=getModeLatency();

		cout<<"Rank "<<m_rank<<": mode latency for "<<(*m_name)<<" when requesting a reply for a message of "<<sizeof(uint64_t)*m_numberOfWords<<" bytes is "<<latency<<" microseconds (10^-6 seconds)"<<endl;

		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		message[0]=latency;
		char*destination=(char*)(message+1);
		strcpy(destination,m_name->c_str());
		Message aMessage(message,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),MASTER_RANK,RAY_MPI_TAG_TEST_NETWORK_REPLY,m_rank);
		m_outbox->push_back(aMessage);

		m_sentData=true;

		m_gotResponse=false;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_TEST_NETWORK_REPLY_REPLY)){
		m_gotResponse=true;

	}else if(m_gotResponse && m_inbox->hasMessage(RAY_MPI_TAG_TEST_NETWORK_WRITE_DATA)){
		// we only write the files, if any, when everyone is done with it
		// otherwise, the measured latency would be higher...
		writeData();

		m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());
	}
}

void NetworkTest::writeData(){
	/* nothing to write */
	if(m_sentMicroseconds.size() == 0)
		return;

	if(m_writeRawData){
		ostringstream file;
		file<<m_parameters->getPrefix();
		file<<"Rank"<<m_parameters->getRank()<<".NetworkTestData.txt";
	
		cout<<"Rank "<<m_parameters->getRank()<<" is writing "<<file.str()<<" (-write-network-test-raw-data)"<<endl;

		ofstream f(file.str().c_str());

		f<<"# tag for test messages: RAY_MPI_TAG_TEST_NETWORK_MESSAGE"<<endl;
		f<<"# tag for reply messages: RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY"<<endl;
		f<<"# number of words per message: "<<m_numberOfWords<<endl;
		f<<"# word size in bytes: "<<sizeof(uint64_t)<<endl;
		f<<"# number of bytes for test messages: "<<m_numberOfWords*sizeof(uint64_t)<<endl;
		f<<"# number of bytes for reply messages: 0"<<endl;
		f<<"# number of test messages: "<<m_numberOfTestMessages<<endl;
		f<<"# mode latency measured in microseconds: "<<getModeLatency()<<endl;
		f<<"# next line contains column names"<<endl;
		f<<"# TestMessage SourceRank DestinationRank QueryTimeInMicroseconds ReplyTimeInMicroseconds Latency MessagesSentToDestination"<<endl;
	
		#ifdef ASSERT
		assert(m_sentMicroseconds.size() == m_destinations.size());
		assert(m_sentMicroseconds.size() == m_receivedMicroseconds.size());
		assert((int) m_sentMicroseconds.size() == m_numberOfTestMessages);
		#endif

		map<int,int> counters;

		for(int i=0;i<(int)m_sentMicroseconds.size();i++){
			uint64_t time1=m_sentMicroseconds[i];
			uint64_t time2=m_receivedMicroseconds[i];
			int destination=m_destinations[i];
			counters[destination] ++ ;
			f<<i<<"	"<<"	"<<m_parameters->getRank()<<"	"<<destination<<"	"<<time1<<"	"<<time2<<"	"<<time2-time1<<"	"<<counters[destination]<<endl;
		}

		f.close();

		ostringstream file2;
		file2<<m_parameters->getPrefix();
		file2<<"Rank"<<m_parameters->getRank()<<".NetworkTestDataCount.txt";
		
		ofstream f2(file2.str().c_str());
		f2<<"# DestinationRank	MessagesSentToDestination"<<endl;
		for(map<int,int>::iterator i=counters.begin();i!=counters.end();i++){
			f2<<i->first<<"	"<<i->second<<endl;
		}

		f2.close();
	}

	m_destinations.clear();
	m_sentMicroseconds.clear();
	m_receivedMicroseconds.clear();
}

/** call the master method */
void NetworkTest::call_RAY_MASTER_MODE_TEST_NETWORK (){
	if(!m_initialisedNetworkTest){
		cout<<"Rank 0: testing the network, please wait..."<<endl;
		cout<<endl;

		m_switchMan->openMasterMode(m_outbox,m_rank);

		m_initialisedNetworkTest=true;
	}else if(m_inbox->size()>0&&(*m_inbox)[0]->getTag()==RAY_MPI_TAG_TEST_NETWORK_REPLY){
		int rank=m_inbox->at(0)->getSource();
		int latency=m_inbox->at(0)->getBuffer()[0];
		uint64_t*buffer=m_inbox->at(0)->getBuffer();
		char*name=(char*)(buffer+1);
		string stringName=name;
		m_names[rank]=stringName;
		m_latencies[rank]=latency;

		m_switchMan->sendEmptyMessage(m_outbox,m_rank,rank,RAY_MPI_TAG_TEST_NETWORK_REPLY_REPLY);

		m_ranksFinished++;

	}else if(m_switchMan->allRanksAreReady()){
		ostringstream file;
		file<<m_parameters->getPrefix();
		file<<"NetworkTest.txt";
		ofstream f(file.str().c_str());
		f<<"# average latency in microseconds (10^-6 seconds) when requesting a reply for a message of "<<sizeof(uint64_t)*m_numberOfWords<<" bytes"<<endl;
		f<<"# MessagePassingInterfaceRank\tName\tModeLatencyInMicroseconds\tNumberOfTestMessages"<<endl;

		vector<int> latencies;
		for(int i=0;i<m_size;i++){
			int latency=m_latencies[i];
			if(latency!=LATENCY_INFORMATION_NOT_AVAILABLE)
				latencies.push_back(latency);
		}

		f<<"# AverageForAllRanks: "<<getAverage(&latencies)<<endl;
		f<<"# StandardDeviation: "<<getStandardDeviation(&latencies)<<endl;

		for(int i=0;i<m_size;i++){
			int latency=m_latencies[i];
			if(latency==LATENCY_INFORMATION_NOT_AVAILABLE){
				f<<i<<"\t"<<m_names[i]<<"\tLATENCY_INFORMATION_NOT_AVAILABLE";
				f<<"	"<<m_numberOfTestMessages<<endl;
			}else{
				f<<i<<"\t"<<m_names[i]<<"\t"<<latency;
				f<<"	"<<m_numberOfTestMessages<<endl;
			}
		}
		f.close();
		m_latencies.clear();

		m_switchMan->closeMasterMode();

		cout<<endl;
		cout<<"Rank "<<m_parameters->getRank()<<" wrote "<<file.str()<<endl;
		cout<<endl;
		m_timePrinter->printElapsedTime("Network testing");
		cout<<endl;
		
		if(m_parameters->hasOption("-test-network-only")){
			m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
			return;
		}

		/* no files */
		if(m_parameters->getNumberOfFiles()==0){
			cout<<"Rank "<<m_parameters->getRank()<<": no input files, aborting."<<endl;
			m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		}
	}else if(m_ranksFinished==m_size && !m_askedToWriteFiles){
		
		m_switchMan->sendToAll(m_outbox,m_parameters->getRank(),RAY_MPI_TAG_TEST_NETWORK_WRITE_DATA);

		m_askedToWriteFiles=true;
	}
}

int NetworkTest::getModeLatency(){
	map<int,int> data;
	int maxLatency=-1;

	for(int i=0;i<(int)m_receivedMicroseconds.size();i++){
		int latency=m_receivedMicroseconds[i]-m_sentMicroseconds[i];
		data[latency]++;

		if(data.count(maxLatency)==0 || data[latency] > data[maxLatency])
			maxLatency=latency;
	}
	
	return maxLatency;
}

void NetworkTest::setSwitchMan(SwitchMan*a){
	m_switchMan=a;
}

void NetworkTest::registerPlugin(ComputeCore*core){

	PluginHandle plugin = core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"NetworkTest");

	RAY_SLAVE_MODE_TEST_NETWORK=core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_TEST_NETWORK);
	m_adapter_RAY_SLAVE_MODE_TEST_NETWORK.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_TEST_NETWORK, &m_adapter_RAY_SLAVE_MODE_TEST_NETWORK);
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_TEST_NETWORK,"RAY_SLAVE_MODE_TEST_NETWORK");

	RAY_MASTER_MODE_TEST_NETWORK=core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_TEST_NETWORK);
	m_adapter_RAY_MASTER_MODE_TEST_NETWORK.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TEST_NETWORK, &m_adapter_RAY_MASTER_MODE_TEST_NETWORK);
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_TEST_NETWORK,"RAY_MASTER_MODE_TEST_NETWORK");

}

void NetworkTest::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_TEST_NETWORK=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_TEST_NETWORK");


	RAY_MASTER_MODE_KILL_ALL_MPI_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_ALL_MPI_RANKS");
	RAY_MASTER_MODE_TEST_NETWORK=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TEST_NETWORK");
}
