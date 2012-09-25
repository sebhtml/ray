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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <plugin_Partitioner/Partitioner.h>
#include <core/OperatingSystem.h>

#include <stdlib.h>

__CreatePlugin(Partitioner);

 /**/
__CreateMasterModeAdapter(Partitioner,RAY_MASTER_MODE_COUNT_FILE_ENTRIES); /**/
 /**/
__CreateSlaveModeAdapter(Partitioner,RAY_SLAVE_MODE_COUNT_FILE_ENTRIES); /**/
 /**/
 /**/


void Partitioner::constructor(RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,
	SwitchMan*switchMan){

	m_switchMan=switchMan;
	m_outboxAllocator=outboxAllocator;
	m_inbox=inbox;
	m_outbox=outbox;
	m_parameters=parameters;
	m_initiatedMaster=false;
	m_initiatedSlave=false;
	m_loader.constructor(m_parameters->getMemoryPrefix().c_str(),m_parameters->showMemoryAllocations());
}

void Partitioner::call_RAY_MASTER_MODE_COUNT_FILE_ENTRIES(){
	/** tell every peer to count entries in files in parallel */
	if(!m_initiatedMaster){
		m_initiatedMaster=true;
		m_ranksDoneCounting=0;
		m_ranksDoneSending=0;
		for(int destination=0;destination<m_parameters->getSize();destination++){
			Message aMessage(NULL,0,destination,RAY_MPI_TAG_COUNT_FILE_ENTRIES,m_parameters->getRank());
			m_outbox->push_back(aMessage);
		}
	/** a peer rank finished counting the entries in its files */
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()== RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY){
		m_ranksDoneCounting++;
		/** all peers have finished */
		if(m_ranksDoneCounting==m_parameters->getSize()){
			for(int destination=0;destination<m_parameters->getSize();destination++){
				Message aMessage(NULL,0,destination,RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS,m_parameters->getRank());
				m_outbox->push_back(aMessage);
			}
		}
	/** a peer send the count for one file */
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()== RAY_MPI_TAG_FILE_ENTRY_COUNT){
		MessageUnit*buffer=m_inbox->at(0)->getBuffer();
		int file=buffer[0];
		LargeCount count=buffer[1];
		m_masterCounts[file]=count;

		if(m_parameters->hasOption("-debug-partitioner"))
			cout<<"Rank "<<m_parameters->getRank()<<" received from "<<m_inbox->at(0)->getSource()<<" File "<<file<<" Entries "<<count<<endl;
		/** reply to the peer */
		Message aMessage(NULL,0,m_inbox->at(0)->getSource(),RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY,m_parameters->getRank());
		m_outbox->push_back(aMessage);
	/** a peer finished sending file counts */
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()== RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY){
		m_ranksDoneSending++;
		/** all peers have finished */
		if(m_ranksDoneSending==m_parameters->getSize()){

			for(int i=0;i<(int)m_masterCounts.size();i++){
				if(m_parameters->hasOption("-debug-partitioner"))
					cout<<"Rank "<<m_parameters->getRank()<< " File "<<i<<" Count "<<m_masterCounts[i]<<endl;
				m_parameters->setNumberOfSequences(i,m_masterCounts[i]);
			}
			m_masterCounts.clear();

			/* write the number of sequences */
			ostringstream fileName;
			fileName<<m_parameters->getPrefix();
			fileName<<"NumberOfSequences.txt";
			ofstream f2(fileName.str().c_str());

/* Write a sequence partition too,
 * it contains the number of entries in each file.
 */
			ostringstream fileNameForFiles;
			fileNameForFiles<<m_parameters->getPrefix();
			fileNameForFiles<<"FilePartition.txt";
			ofstream partitionStream(fileNameForFiles.str().c_str());

			f2<<"Files: "<<m_parameters->getNumberOfFiles()<<endl;
			f2<<endl;

			partitionStream<<"#File	Name	FirstSequence	LastSequence	NumberOfSequences"<<endl;

			LargeCount totalSequences=0;
			for(int i=0;i<(int)m_parameters->getNumberOfFiles();i++){
				f2<<"FileNumber: "<<i<<endl;
				f2<<"	FilePath: "<<m_parameters->getFile(i)<<endl;

				LargeCount entries=m_parameters->getNumberOfSequences(i);
				f2<<" 	NumberOfSequences: "<<entries<<endl;

				if(entries>0){
					f2<<"	FirstSequence: "<<totalSequences<<endl;
					f2<<"	LastSequence: "<<totalSequences+entries-1<<endl;
				}

				f2<<endl;

				if(entries>0){
					partitionStream<<i<<"	"<<m_parameters->getFile(i);
					partitionStream<<"	"<<totalSequences;
					partitionStream<<"	"<<totalSequences+entries-1;
					partitionStream<<"	"<<entries<<endl;
				}

				totalSequences+=entries;
			}
			
			partitionStream.close();

			f2<<endl;
			f2<<"Summary"<<endl;
			f2<<"	NumberOfSequences: "<<totalSequences<<endl;
			f2<<"	FirstSequence: 0"<<endl;
			f2<<"	LastSequence: "<<totalSequences-1<<endl;
			f2.close();
			cout<<"Rank "<<m_parameters->getRank()<<" wrote "<<fileName.str()<<endl;


			/* write the partition */
			ostringstream fileName2;
			fileName2<<m_parameters->getPrefix();
			fileName2<<"SequencePartition.txt";
			ofstream f3(fileName2.str().c_str());

			LargeCount perRank=totalSequences/m_parameters->getSize();
			f3<<"#Rank	FirstSequence	LastSequence	NumberOfSequences"<<endl;
			for(int i=0;i<m_parameters->getSize();i++){
				LargeIndex first=i*perRank;
				LargeIndex last=first+perRank-1;

				if(i==m_parameters->getSize()-1){
					last=totalSequences-1;
				}
				
				LargeCount count=last-first+1;

				f3<<i<<"\t"<<first<<"\t"<<last<<"\t"<<count<<endl;
			}
			f3.close();
			cout<<"Rank "<<m_parameters->getRank()<<" wrote "<<fileName2.str()<<endl;

			m_switchMan->closeMasterMode();
		}
	}
}

void Partitioner::call_RAY_SLAVE_MODE_COUNT_FILE_ENTRIES(){
	/** initialize the slave */
	if(!m_initiatedSlave){
		m_initiatedSlave=true;
		m_currentFileToCount=0;
		m_currentlySendingCounts=false;
		m_sentCount=false;
		
		/* possibly read the checkpoint */
		if(m_parameters->hasCheckpoint("Partition")){
			ifstream f(m_parameters->getCheckpointFile("Partition").c_str());
			cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint Partition"<<endl;
			int count=0;
			f.read((char*)&count,sizeof(int));
			for(int i=0;i<count;i++){
				int file=-1;
				LargeCount sequences=0;
				f.read((char*)&file,sizeof(int));
				f.read((char*)&sequences,sizeof(LargeCount));

				#ifdef ASSERT
				assert(file>=0);
				assert(m_slaveCounts.count(file)==0);
				#endif
			
				m_slaveCounts[file]=sequences;

				#ifdef ASSERT
				assert(m_slaveCounts.count(file)>0);
				#endif
				cout<<"Rank "<<m_parameters->getRank()<<": from checkpoint Partition, file "<<file<<" has "<<sequences<<" sequences."<<endl;
			}
			f.close();
			m_currentFileToCount=m_parameters->getNumberOfFiles();
		}
	/* all files were processed, tell control peer that we are done */
	}else if(m_currentFileToCount==m_parameters->getNumberOfFiles()){
		Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY,m_parameters->getRank());
		m_outbox->push_back(aMessage);
		/* increment it so we don't go here again. */
		m_currentFileToCount++;

		/* Here we write the checkpoint Partition */
		if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("Partition")){
			ofstream f(m_parameters->getCheckpointFile("Partition").c_str());
			cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint Partition"<<endl;
			int count=m_slaveCounts.size();
			f.write((char*)&count,sizeof(int));

			for(map<int,LargeCount>::iterator i=m_slaveCounts.begin();
				i!=m_slaveCounts.end();i++){
				int file=i->first;
				LargeCount sequences=i->second;
				f.write((char*)&file,sizeof(int));
				f.write((char*)&sequences,sizeof(LargeCount));
			}

			f.close();
		}
	/** count sequences in a file */
	}else if(m_currentFileToCount<m_parameters->getNumberOfFiles()){
		int rankInCharge=m_currentFileToCount%m_parameters->getSize();
		if(rankInCharge==m_parameters->getRank()){
			/** count the entries in the file */
			string file=m_parameters->getFile(m_currentFileToCount);
			//cout<<"Rank "<<m_parameters->getRank()<<" Reading "<<file<<endl;
			int res=m_loader.load(file,false);
			if(res==EXIT_FAILURE){
				cout<<"Rank "<<m_parameters->getRank()<<" Error: "<<file<<" failed to load properly..."<<endl;
			}
			m_slaveCounts[m_currentFileToCount]=m_loader.size();

			m_loader.clear();

			cout<<"Rank "<<m_parameters->getRank()<<": File "<<file<<" (Number "<<m_currentFileToCount<<") has "<<m_slaveCounts[m_currentFileToCount]<<" sequences"<<endl;
		}
		m_currentFileToCount++;

	/** control peer asks the slave to send counts */
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag() == RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS){
		m_currentFileToSend=0;
		m_currentlySendingCounts=true;
	/** sending counts */
	}else if(m_currentlySendingCounts){
		if(m_currentFileToSend<m_parameters->getNumberOfFiles()){
			int rankInCharge=m_currentFileToSend%m_parameters->getSize();
			/** skip the file, we are not in charge */
			if(rankInCharge!=m_parameters->getRank()){
				m_currentFileToSend++;
			/** send the count and wait for a reply to continue */
			}else if(!m_sentCount){
				MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
				message[0]=m_currentFileToSend;
				message[1]=m_slaveCounts[m_currentFileToSend];
				Message aMessage(message,2,MASTER_RANK,RAY_MPI_TAG_FILE_ENTRY_COUNT,m_parameters->getRank());
				m_outbox->push_back(aMessage);
				m_sentCount=true;
			/** we got a reply, let's continue */
			}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag() == RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY){
				m_sentCount=false;
				m_currentFileToSend++;
			}
		/** all counts were processed, report this to the control peer */
		}else{
			Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY,m_parameters->getRank());
			m_outbox->push_back(aMessage);
			m_slaveCounts.clear();

			m_switchMan->setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
		}
	}
}

void Partitioner::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"Partitioner");
	core->setPluginDescription(plugin,"A plugin that counts sequences in files in parallel");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_MASTER_MODE_COUNT_FILE_ENTRIES=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_COUNT_FILE_ENTRIES,__GetAdapter(Partitioner,RAY_MASTER_MODE_COUNT_FILE_ENTRIES));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_COUNT_FILE_ENTRIES,"RAY_MASTER_MODE_COUNT_FILE_ENTRIES");

	RAY_SLAVE_MODE_COUNT_FILE_ENTRIES=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_COUNT_FILE_ENTRIES, __GetAdapter(Partitioner,RAY_SLAVE_MODE_COUNT_FILE_ENTRIES));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_COUNT_FILE_ENTRIES,"RAY_SLAVE_MODE_COUNT_FILE_ENTRIES");

	RAY_MPI_TAG_COUNT_FILE_ENTRIES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_COUNT_FILE_ENTRIES,"RAY_MPI_TAG_COUNT_FILE_ENTRIES");

	RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY,"RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY");

	RAY_MPI_TAG_FILE_ENTRY_COUNT=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_FILE_ENTRY_COUNT,"RAY_MPI_TAG_FILE_ENTRY_COUNT");

	RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY,"RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY");

	RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS,"RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS");

	RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY,"RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY");

}

void Partitioner::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");
	RAY_SLAVE_MODE_COUNT_FILE_ENTRIES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_COUNT_FILE_ENTRIES");

	RAY_MASTER_MODE_COUNT_FILE_ENTRIES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_COUNT_FILE_ENTRIES");
	RAY_MASTER_MODE_LOAD_SEQUENCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_LOAD_SEQUENCES");

	RAY_MPI_TAG_COUNT_FILE_ENTRIES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COUNT_FILE_ENTRIES");
	RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY");
	RAY_MPI_TAG_FILE_ENTRY_COUNT=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_FILE_ENTRY_COUNT");
	RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY");
	RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS");
	RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY");

	RAY_MPI_TAG_COUNT_FILE_ENTRIES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COUNT_FILE_ENTRIES");
	RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY");

	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_COUNT_FILE_ENTRIES, RAY_SLAVE_MODE_COUNT_FILE_ENTRIES );

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_COUNT_FILE_ENTRIES,RAY_MASTER_MODE_LOAD_SEQUENCES);

	__BindPlugin(Partitioner);

}
