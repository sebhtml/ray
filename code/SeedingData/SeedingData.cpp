/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2010, 2011, 2012, 2013 Sébastien Boisvert

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

#include "SeedingData.h"

#include <code/Mock/constants.h>
#include <code/SeedingData/SeedWorker.h>

#include <RayPlatform/communication/Message.h>
#include <RayPlatform/communication/VirtualCommunicator.h>
#include <RayPlatform/communication/mpi_tags.h>
#include <RayPlatform/core/OperatingSystem.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <assert.h>

__CreatePlugin(SeedingData);

__CreateSlaveModeAdapter(SeedingData,RAY_SLAVE_MODE_START_SEEDING);
__CreateSlaveModeAdapter(SeedingData,RAY_SLAVE_MODE_SEND_SEED_LENGTHS);

/*
 * TODO: port this with the VirtualProcessor framework.
 */


void SeedingData::call_RAY_SLAVE_MODE_START_SEEDING(){
	if(!m_initiatedIterator){
		m_last=time(NULL);

		m_SEEDING_i=0;

		m_activeWorkerIterator=m_activeWorkers.begin();
		m_splayTreeIterator.constructor(m_subgraph,m_wordSize,m_parameters);
		m_initiatedIterator=true;
		m_maximumAliveWorkers=32768;

		#ifdef CONFIG_ASSERT
		m_splayTreeIterator.hasNext();
		#endif


		m_virtualCommunicator->resetCounters();
	}

	if(!m_checkedCheckpoint){
		if(m_parameters->hasCheckpoint("SimpleSeeds")){
			cout<<"Rank "<<m_parameters->getRank()<<": checkpoint SimpleSeeds exists, not computing seeds."<<endl;
			(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
			Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_SEEDING_IS_OVER,getRank());
			m_outbox->push_back(&aMessage);

			loadCheckpoint();

			return;
		}
		m_checkedCheckpoint=true;
	}

	m_virtualCommunicator->processInbox(&m_activeWorkersToRestore);

	if(!m_virtualCommunicator->isReady()){
		return;
	}

	// flush all mode is necessary to empty buffers and 
	// restart things from scratch..

	// 1. iterate on active workers
	if(m_activeWorkerIterator!=m_activeWorkers.end()){
		WorkerHandle workerId=*m_activeWorkerIterator;
		#ifdef CONFIG_ASSERT
		assert(m_aliveWorkers.count(workerId)>0);
		assert(!m_aliveWorkers[workerId].isDone());
		#endif
		m_virtualCommunicator->resetLocalPushedMessageStatus();

		//force the worker to work until he finishes or pushes something on the stack
		while(!m_aliveWorkers[workerId].isDone()&&!m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_aliveWorkers[workerId].work();
		}

		if(m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_waitingWorkers.push_back(workerId);
		}
		if(m_aliveWorkers[workerId].isDone()){
			m_workersDone.push_back(workerId);
			GraphPath*seed=m_aliveWorkers[workerId].getSeed();

			int minimalSeedLength = 100;

			if(m_parameters->hasConfigurationOption("-minimum-seed-length", 1)) {
				minimalSeedLength = m_parameters->getConfigurationInteger("-minimum-seed-length", 0);
			}

			int nucleotides=getNumberOfNucleotides(seed->size(),m_wordSize);

			if(seed->size() > 0 && m_debugSeeds){
				cout<<"Raw seed length: "<<nucleotides<<" nucleotides"<<endl;
			}

			#ifdef CONFIG_ASSERT
			assert(nucleotides==0 || nucleotides>=m_wordSize);
			#endif

			SeedWorker*worker=&(m_aliveWorkers[workerId]);

			bool isSmall = nucleotides < 4 * m_parameters->getWordSize();

			if(isSmall && worker->isHeadADeadEnd() && worker->isTailADeadEnd()){

				m_skippedObjectsWithTwoDeadEnds++;

			}else if(isSmall && worker->isHeadADeadEnd()){

				m_skippedObjectsWithDeadEndForHead++;

			}else if(isSmall && worker->isTailADeadEnd()){

				m_skippedObjectsWithDeadEndForTail++;

			}else if(isSmall && worker->isBubbleWeakComponent()){

				m_skippedObjectsWithBubbleWeakComponent++;

			// only consider the long ones.
			}else if(nucleotides >= minimalSeedLength){

				#ifdef SHOW_DISCOVERIES
				printf("Rank %i discovered a seed with %i vertices\n",m_rank,(int)seed.size());
				#endif

				#ifdef CONFIG_ASSERT
				assert(seed->size()>0);
				#endif

				Kmer firstVertex;
				seed->at(0,&firstVertex);
				Kmer lastVertex;
				seed->at(seed->size()-1,&lastVertex);
				Kmer firstReverse=m_parameters->_complementVertex(&lastVertex);

				int minimumNucleotidesForVerbosity=1024;

				bool verbose=nucleotides>=minimumNucleotidesForVerbosity;

				if(m_debugSeeds){
					verbose=true;
				}

				bool ignoreSeeds = m_parameters->hasOption("-ignore-seeds");

				if(firstVertex<firstReverse && !ignoreSeeds){

					if(verbose){
						printf("Rank %i stored a seed with %i vertices\n",m_rank,(int)seed->size());
					}

					if(m_parameters->showMemoryUsage() && verbose){
						showMemoryUsage(m_rank);
					}

					GraphPath*theSeed=seed;

					theSeed->computePeakCoverage();

					CoverageDepth peakCoverage=theSeed->getPeakCoverage();

					if(verbose)
						cout<<"Got a seed, peak coverage: "<<peakCoverage;

					/* ignore the seed if it has too much coverage. */
					if(peakCoverage >= m_minimumSeedCoverageDepth
						&& peakCoverage <= m_parameters->getMaximumSeedCoverage()){

						if(verbose)
							cout<<", adding seed."<<endl;

						m_SEEDING_seeds.push_back(*theSeed);

						m_eligiblePaths++;
					}else{

						if(verbose)
							cout<<", ignoring seed."<<endl;

						m_skippedNotEnoughCoverage++;
					}
				}else{
					m_skippedNotMine++;
				}
			}else{
				m_skippedTooShort++;
			}
		}
		m_activeWorkerIterator++;
	}else{
		updateStates();

		//  add one worker to active workers
		//  reason is that those already in the pool don't communicate anymore -- 
		//  as for they need responses.
		if(!m_virtualCommunicator->getGlobalPushedMessageStatus()&&m_activeWorkers.empty()){
			// there is at least one worker to start
			// AND
			// the number of alive workers is below the maximum
			if(m_SEEDING_i<m_subgraph->size()&&(int)m_aliveWorkers.size()<m_maximumAliveWorkers){
				if(m_SEEDING_i % 100000 ==0){
					printf("Rank %i is creating seeds [%i/%i]\n",getRank(),(int)m_SEEDING_i+1,(int)m_subgraph->size());

					if(m_parameters->showMemoryUsage()){
						showMemoryUsage(m_rank);
					}
				}

				#ifdef CONFIG_ASSERT
				if(m_SEEDING_i==0){
					assert(m_completedJobs==0&&m_activeWorkers.size()==0&&m_aliveWorkers.size()==0);
				}
				#endif

				m_splayTreeIterator.next();
				Kmer vertexKey=*(m_splayTreeIterator.getKey());

				m_aliveWorkers[m_SEEDING_i].constructor(&vertexKey,m_parameters,m_outboxAllocator,m_virtualCommunicator,m_SEEDING_i,
RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE
);
				if(m_debugSeeds)
					m_aliveWorkers[m_SEEDING_i].enableDebugMode();

				m_activeWorkers.insert(m_SEEDING_i);

				int population=m_aliveWorkers.size();
				if(population>m_maximumWorkers){
					m_maximumWorkers=population;
				}

				m_SEEDING_i++;

				// skip the reverse complement as we don't really need it anyway.
			}else{
				m_virtualCommunicator->forceFlush();
			}
		}

		// brace yourself for the next round
		m_activeWorkerIterator=m_activeWorkers.begin();
	}

	#ifdef CONFIG_ASSERT
	assert((int)m_aliveWorkers.size()<=m_maximumAliveWorkers);
	#endif

	if((int)m_subgraph->size()==m_completedJobs){

		printf("Rank %i has %i seeds\n",m_rank,(int)m_SEEDING_seeds.size());
		printf("Rank %i is creating seeds [%i/%i] (completed)\n",getRank(),(int)m_SEEDING_i,(int)m_subgraph->size());
		printf("Rank %i: peak number of workers: %i, maximum: %i\n",m_rank,m_maximumWorkers,m_maximumAliveWorkers);
		m_virtualCommunicator->printStatistics();

		cout<<"Rank "<<m_rank<<" runtime statistics for seeding algorithm: "<<endl;
		cout<<"Rank "<<m_rank<<" Skipped paths because of dead end for head: "<<m_skippedObjectsWithDeadEndForHead<<endl;
		cout<<"Rank "<<m_rank<<" Skipped paths because of dead end for tail: "<<m_skippedObjectsWithDeadEndForTail<<endl;
		cout<<"Rank "<<m_rank<<" Skipped paths because of two dead ends: "<<m_skippedObjectsWithTwoDeadEnds<<endl;
		cout<<"Rank "<<m_rank<<" Skipped paths because of bubble weak component: "<<m_skippedObjectsWithBubbleWeakComponent<<endl;
		cout<<"Rank "<<m_rank<<" Skipped paths because of short length: "<<m_skippedTooShort<<endl;
		cout<<"Rank "<<m_rank<<" Skipped paths because of bad ownership: "<<m_skippedNotMine<<endl;
		cout<<"Rank "<<m_rank<<" Skipped paths because of low coverage: "<<m_skippedNotEnoughCoverage<<endl;
		cout<<"Rank "<<m_rank<<" Eligible paths: "<<m_eligiblePaths<<endl;

		#ifdef CONFIG_ASSERT
		assert(m_eligiblePaths==(int)m_SEEDING_seeds.size());
		#endif

		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_SEEDING_IS_OVER,getRank());
		m_outbox->push_back(&aMessage);

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_rank);
		}

		#ifdef CONFIG_ASSERT
		assert(m_aliveWorkers.size()==0);
		assert(m_activeWorkers.size()==0);
		#endif

		// sort the seeds by length
		std::sort(m_SEEDING_seeds.begin(),
			m_SEEDING_seeds.end(), comparePaths);

		/**************************************************************
		 * Write down the SimpleSeeds checkpoint now.
		 **********************************************************************/

		writeCheckpoints();
	}
}

void SeedingData::constructor(SeedExtender*seedExtender,int rank,int size,StaticVector*outbox,RingAllocator*outboxAllocator,
int*mode,
	Parameters*parameters,GridTable*subgraph,StaticVector*inbox,
	VirtualCommunicator*vc){

	m_skippedObjectsWithDeadEndForHead=0;
	m_skippedObjectsWithDeadEndForTail=0;
	m_skippedObjectsWithTwoDeadEnds=0;
	m_skippedObjectsWithBubbleWeakComponent=0;

	m_skippedTooShort=0;
	m_skippedNotMine=0;
	m_skippedNotEnoughCoverage=0;
	m_eligiblePaths=0;

	m_checkedCheckpoint=false;
	m_virtualCommunicator=vc;
	m_seedExtender=seedExtender;
	m_size=size;
	m_rank=rank;
	m_outbox=outbox;
	m_inbox=inbox;
	m_completedJobs=0;
	m_maximumWorkers=0;
	m_flushAllMode=false;
	m_outboxAllocator=outboxAllocator;
	m_mode=mode;
	m_parameters=parameters;
	m_wordSize=m_parameters->getWordSize();
	#ifdef CONFIG_ASSERT
	assert(m_wordSize>=15&&m_wordSize<=CONFIG_MAXKMERLENGTH);
	#endif
	m_subgraph=subgraph;
	m_initiatedIterator=false;

	m_minimumSeedCoverageDepth=0;

	if(m_parameters->hasConfigurationOption("-use-minimum-seed-coverage",1)){
		m_minimumSeedCoverageDepth=m_parameters->getConfigurationInteger("-use-minimum-seed-coverage",0);
		cout<<"[SeedingData] will use "<<m_minimumSeedCoverageDepth<<" for the minimum seed coverage"<<endl;
	}

	m_debugSeeds=false;

	if(m_parameters->hasConfigurationOption("-debug-seeds",0))
		m_debugSeeds=true;
}

int SeedingData::getRank(){
	return m_rank;
}

int SeedingData::getSize(){
	return m_size;
}

void SeedingData::updateStates(){
	// erase completed jobs
	for(int i=0;i<(int)m_workersDone.size();i++){
		WorkerHandle workerId=m_workersDone[i];

		#ifdef CONFIG_ASSERT
		assert(m_activeWorkers.count(workerId)>0);
		assert(m_aliveWorkers.count(workerId)>0);
		#endif

		m_activeWorkers.erase(workerId);
		m_aliveWorkers.erase(workerId);
		m_completedJobs++;
	}
	m_workersDone.clear();

	for(int i=0;i<(int)m_waitingWorkers.size();i++){
		WorkerHandle workerId=m_waitingWorkers[i];
		#ifdef CONFIG_ASSERT
		assert(m_activeWorkers.count(workerId)>0);
		#endif
		m_activeWorkers.erase(workerId);
	}
	m_waitingWorkers.clear();

	for(int i=0;i<(int)m_activeWorkersToRestore.size();i++){
		WorkerHandle workerId=m_activeWorkersToRestore[i];
		m_activeWorkers.insert(workerId);
	}
	m_activeWorkersToRestore.clear();

	m_virtualCommunicator->resetGlobalPushedMessageStatus();
}

void SeedingData::call_RAY_SLAVE_MODE_SEND_SEED_LENGTHS(){
	if(!m_initialized){

		m_virtualCommunicator->resetCounters();

		m_initialized = true;
	}

	Message aMessage(NULL,0,MASTER_RANK,
		RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS,getRank());
	m_outbox->push_back(&aMessage);
	(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
}

void SeedingData::loadCheckpoint(){
	cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint SimpleSeeds"<<endl;

	ifstream f(m_parameters->getCheckpointFile("SimpleSeeds").c_str());
	int n=0;
	f.read((char*)&n,sizeof(int));
	for(int i=0;i<n;i++){
		GraphPath seed;
		seed.setKmerLength(m_parameters->getWordSize());
		int vertices=0;
		f.read((char*)&vertices,sizeof(int));
		for(int j=0;j<vertices;j++){
			Kmer kmer;
			kmer.read(&f);
			seed.push_back(&kmer);

			CoverageDepth coverageValue=0;

			f.read((char*)&coverageValue,sizeof(CoverageDepth));
			seed.addCoverageValue(coverageValue);
		}

		seed.computePeakCoverage();

		m_SEEDING_seeds.push_back(seed);
	}
	cout<<"Rank "<<m_parameters->getRank()<<" loaded "<<n<<" seeds from checkpoint SimpleSeeds"<<endl;
	f.close();
}

void SeedingData::writeCheckpoints(){

	/* write the Seeds checkpoint */
	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("SimpleSeeds")){

		ofstream f(m_parameters->getCheckpointFile("SimpleSeeds").c_str());
		ostringstream buffer;

		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint SimpleSeeds"<<endl;

		vector<GraphPath> * seeds = & m_SEEDING_seeds;

		int count=(*seeds).size();

		buffer.write((char*)&count, sizeof(int));

		for(int i=0;i<(int)(*seeds).size();i++){
			int length=(*seeds)[i].size();
			buffer.write((char*)&length, sizeof(int));

			for(int j=0;j<(int)(*seeds)[i].size();j++){
				Kmer theKmer;
				(*seeds)[i].at(j,&theKmer);
				theKmer.write(&buffer);

				CoverageDepth coverageValue=0;
				coverageValue=(*seeds)[i].getCoverageAt(j);
				buffer.write((char*)&coverageValue, sizeof(CoverageDepth));
				flushFileOperationBuffer(false, &buffer, &f, CONFIG_FILE_IO_BUFFER_SIZE);
			}
		}
                flushFileOperationBuffer(true, &buffer, &f, CONFIG_FILE_IO_BUFFER_SIZE);
		f.close();
	}
}



void SeedingData::registerPlugin(ComputeCore*core){

	m_core=core;
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"SeedingData");
	core->setPluginDescription(plugin,"Computes unique paths in the graph");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	__ConfigureSlaveModeHandler(SeedingData, RAY_SLAVE_MODE_START_SEEDING);
	__ConfigureSlaveModeHandler(SeedingData, RAY_SLAVE_MODE_SEND_SEED_LENGTHS);

	RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY,"RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY");

	__BindPlugin(SeedingData);

	m_core->setObjectSymbol(m_plugin, &m_SEEDING_seeds,"/RayAssembler/ObjectStore/Seeds.ray");
}

void SeedingData::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_START_SEEDING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_START_SEEDING");
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");
	RAY_SLAVE_MODE_SEND_SEED_LENGTHS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_SEED_LENGTHS");

	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE");

	RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS");
	RAY_MPI_TAG_SEEDING_IS_OVER=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEEDING_IS_OVER");
	RAY_MPI_TAG_SEND_SEED_LENGTHS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEND_SEED_LENGTHS");
	RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY");
}
