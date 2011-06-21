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
#include <assembler/FusionData.h>
#include <sstream>
#include <communication/Message.h>
#include <memory/malloc_types.h>
using namespace std;

#define SHOW_FUSION

void FusionData::distribute(SeedingData*m_seedingData,ExtensionData*m_ed,int getRank,RingAllocator*m_outboxAllocator,StaticVector*m_outbox,int getSize1,int*m_mode){
	if(!isReady()){
		return;
	}
	if(!m_buffers.isEmpty() && m_seedingData->m_SEEDING_i==(uint64_t)m_ed->m_EXTENSION_contigs.size()){
		m_ready+=m_buffers.flushAll(RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY,m_outboxAllocator,m_outbox,getRank);
		return;
	}else if(m_buffers.isEmpty() && m_seedingData->m_SEEDING_i==(uint64_t)m_ed->m_EXTENSION_contigs.size()){
		printf("Rank %i is distributing fusions [%i/%i] (completed)\n",getRank,(int)m_ed->m_EXTENSION_contigs.size(),(int)m_ed->m_EXTENSION_contigs.size());
		fflush(stdout);
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED,getRank);
		m_outbox->push_back(aMessage);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		m_buffers.clear();

		m_cacheForRepeatedVertices.clear();
		m_cacheAllocator.clear();

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_rank);
			now();
		}
		return;
	}

	if(m_ed->m_EXTENSION_currentPosition==0){
		if(m_seedingData->m_SEEDING_i%10==0){
			printf("Rank %i is distributing fusions [%i/%i]\n",getRank,(int)(m_seedingData->m_SEEDING_i+1),(int)m_ed->m_EXTENSION_contigs.size());
			fflush(stdout);
			if(m_parameters->showMemoryUsage()){
				showMemoryUsage(getRank);
				now();
			}
		}
	}

	#ifdef ASSERT
	assert(m_seedingData->m_SEEDING_i<m_ed->m_EXTENSION_contigs.size());
	#endif

	Kmer vertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
	int destination=m_parameters->_vertexRank(&vertex);
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		m_buffers.addAt(destination,vertex.getU64(i));
	}
	m_buffers.addAt(destination,m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i]);
	m_buffers.addAt(destination,m_ed->m_EXTENSION_currentPosition);

	if(m_buffers.flush(destination,KMER_U64_ARRAY_SIZE+2,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY,m_outboxAllocator,m_outbox,getRank,false)){
		m_ready++;
	}

	m_ed->m_EXTENSION_currentPosition++;

	// the next one
	if(m_ed->m_EXTENSION_currentPosition==(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
		m_seedingData->m_SEEDING_i++;
		m_ed->m_EXTENSION_currentPosition=0;
	}
}

void FusionData::readyBuffers(){
	m_buffers.constructor(m_size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),
		RAY_MALLOC_TYPE_FUSION_BUFFERS,m_parameters->showMemoryAllocations());
}

void FusionData::constructor(int size,int max,int rank,StaticVector*outbox,
		RingAllocator*outboxAllocator,int wordSize,
		ExtensionData*ed,SeedingData*seedingData,int*mode,Parameters*parameters){
	m_timer.constructor();
	m_parameters=parameters;
	m_seedingData=seedingData;
	ostringstream prefixFull;
	prefixFull<<m_parameters->getMemoryPrefix()<<"_FusionData";
	m_cacheAllocator.constructor(4194304,RAY_MALLOC_TYPE_FUSION_CACHING,m_parameters->showMemoryAllocations());
	m_cacheForRepeatedVertices.constructor();
	m_mode=mode;
	m_ed=ed;
	m_size=size;
	m_rank=rank;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_wordSize=wordSize;
	#ifdef ASSERT
	assert(m_wordSize>0);
	#endif
	
	m_FINISH_pathsForPosition=new vector<vector<Direction> >;
	m_mappingConfirmed=false;
	m_validationPosition=0;
	m_Machine_getPaths_INITIALIZED=false;
	m_Machine_getPaths_DONE=false;
}

int FusionData::getRank(){
	return m_rank;
}

int FusionData::getSize(){
	return m_size;
}

FusionData::FusionData(){
	m_ready=0;
}

void FusionData::setReadiness(){
	m_ready--;
}

bool FusionData::isReady(){
	return m_ready==0;
}

/*
 * find overlap between extensions
 *
 * example:
 *
 *
 *
 *         ----------------------->
 *                          ----------------------->
 */
void FusionData::finishFusions(){
	if(m_seedingData->m_SEEDING_i==(uint64_t)m_ed->m_EXTENSION_contigs.size()){
		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
		message[0]=m_FINISH_fusionOccured;
		printf("Rank %i is finishing fusions [%i/%i] (completed)\n",getRank(),(int)m_ed->m_EXTENSION_contigs.size(),(int)m_ed->m_EXTENSION_contigs.size());
		fflush(stdout);
		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_rank);
			now();
		}

		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_FINISH_FUSIONS_FINISHED,getRank());
		m_outbox->push_back(aMessage);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;

		m_cacheForRepeatedVertices.clear();
		m_cacheAllocator.clear();

		#ifdef ASSERT
		assert(m_FINISH_pathsForPosition!=NULL);
		#endif
		delete m_FINISH_pathsForPosition;
		m_FINISH_pathsForPosition=new vector<vector<Direction> >;
		return;
	}
	int overlapMinimumLength=2000;
	if((int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<overlapMinimumLength){
	
		if(m_seedingData->m_SEEDING_i%10==0){
			printf("Rank %i is finishing fusions [%i/%i]\n",getRank(),(int)m_seedingData->m_SEEDING_i+1,(int)m_ed->m_EXTENSION_contigs.size());
			fflush(stdout);
	
			if(m_parameters->showMemoryUsage()){
				showMemoryUsage(getRank());
				now();
			}
		}

		m_seedingData->m_SEEDING_i++;
		m_FINISH_vertex_requested=false;
		m_ed->m_EXTENSION_currentPosition=0;
		m_FUSION_pathLengthRequested=false;
		m_Machine_getPaths_INITIALIZED=false;
		m_Machine_getPaths_DONE=false;
		m_checkedValidity=false;

		return;
	}
	// check if the path begins with someone else.
	
	uint64_t currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
	#ifdef ASSERT
	assert(getRankFromPathUniqueId(currentId)<m_size);
	#endif

	// don't do it if it is removed.

	// start threading the extension
	// as the algorithm advance on it, it stores the path positions.
	// when it reaches a choice, it will use the available path as basis.
	
	// we have the extension in m_ed->m_EXTENSION_contigs[m_SEEDING_i]
	// we get the paths with getPaths
	bool done=false;

	int capLength=80;
	int position1=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-1-capLength;
	int position2=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-overlapMinimumLength+capLength;
	if(m_ed->m_EXTENSION_currentPosition<(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
		if(!m_Machine_getPaths_DONE){
			if(m_ed->m_EXTENSION_currentPosition!=position1	&&m_ed->m_EXTENSION_currentPosition!=position2){
				m_Machine_getPaths_DONE=true;
				m_Machine_getPaths_result.clear();// avoids major leak... LOL
			}else{
				getPaths(m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition]);
			}
		}else{
			// at this point, we have the paths that has the said vertex in them.
			// remove selfId.
			vector<Direction> a;
			for(int i=0;i<(int)m_Machine_getPaths_result.size();i++){
				if(m_Machine_getPaths_result[i].getWave()!=currentId){
					a.push_back(m_Machine_getPaths_result[i]);
				}
			}
			m_FINISH_pathsForPosition->push_back(a);
			m_FINISH_coverages.push_back(m_seedingData->m_SEEDING_receivedVertexCoverage);
			if(m_ed->m_EXTENSION_currentPosition==0){
				if(m_seedingData->m_SEEDING_i%10==0){
					printf("Rank %i is finishing fusions [%i/%i]\n",getRank(),(int)m_seedingData->m_SEEDING_i+1,(int)m_ed->m_EXTENSION_contigs.size());
					fflush(stdout);
	
					if(m_parameters->showMemoryUsage()){
						showMemoryUsage(getRank());
						now();
					}
				}
				vector<Kmer> a;
				m_FINISH_newFusions.push_back(a);
				vector<int> b;
				m_FINISH_coverages.clear();
				m_FINISH_vertex_requested=false;
				m_FUSION_eliminated.insert(currentId);
				m_FUSION_pathLengthRequested=false;
				m_checkedValidity=false;
			}
			Kmer vertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
			m_FINISH_newFusions[m_FINISH_newFusions.size()-1].push_back(vertex);
			m_Machine_getPaths_DONE=false;
			m_Machine_getPaths_INITIALIZED=false;
			m_Machine_getPaths_result.clear();
			m_ed->m_EXTENSION_currentPosition++;
		}
	}else if(!m_checkedValidity){

		done=true;
		vector<Direction> directions1=(*m_FINISH_pathsForPosition)[position1];
		vector<Direction> directions2=(*m_FINISH_pathsForPosition)[position2];

		// no hits are possible.
		if(directions1.size()==0 || directions2.size()==0){
			m_checkedValidity=true;
		}else{

		// basically, directions1 contains the paths at a particular vertex in the path
		// directions2 contains the paths at another vertex in the path
		// both vertices are distanced by overlapMinimumLength, or so
		// basically, here we say we have a hit if and only if
		// there is a pair x,y with x in directions1 ad y in directions2
		// with the property that the difference of progressions are exactly overlapMinimumLength (progressions
		// are simply positions of these vertices on another path.)
		// 

			int hits=0;
			map<uint64_t,vector<int> > indexOnDirection2;

			set<uint64_t> in1;
			
			for(int j=0;j<(int)directions1.size();j++){
				uint64_t waveId=directions1[j].getWave();
				in1.insert(waveId);
			}

			// index the index for each wave
			for(int j=0;j<(int)directions2.size();j++){
				uint64_t waveId=directions2[j].getWave();
				if(in1.count(waveId)==0){
					continue;
				}
				if(indexOnDirection2.count(waveId)==0){
					vector<int> emptyVector;
					indexOnDirection2[waveId]=emptyVector;
				}
				indexOnDirection2[waveId].push_back(j);
			}
	
			// find all hits
			//
			for(int i=0;i<(int)directions1.size();i++){
				uint64_t wave1=directions1[i].getWave();
				if(indexOnDirection2.count(wave1)==0){
					continue;
				}
				vector<int> searchResults=indexOnDirection2[wave1];
				int progression1=directions1[i].getProgression();
				for(int j=0;j<(int)searchResults.size();j++){
					int index2=searchResults[j];
					int otherProgression=directions2[index2].getProgression();
					int observedDistance=(progression1-otherProgression+1);
					int expectedDistance=(overlapMinimumLength-2*capLength);
					if(observedDistance==expectedDistance){
						// this is 
						done=false;
						hits++;
						m_selectedPath=wave1;
						m_selectedPosition=progression1+capLength;
					}
				}
			}

			indexOnDirection2.clear();
	
			/**
 	*		if there is more than one hit, they must be repeated regions. (?)
 	*
 	*/
			if(hits>1){// we don't support that right now.
				done=true;
			}	


			m_checkedValidity=true;
		}
	}else if(!m_mappingConfirmed){
		if(position1<=m_validationPosition && m_validationPosition<=position2){
			if(!m_Machine_getPaths_DONE){
				#ifdef ASSERT
				assert(m_seedingData->m_SEEDING_i<m_ed->m_EXTENSION_contigs.size());
				assert(m_ed->m_EXTENSION_currentPosition<(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size());
				#endif
				getPaths(m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition]);
			}else{

				bool found=false;
				for(int i=0;i<(int)m_Machine_getPaths_result.size();i++){
					if(m_Machine_getPaths_result[i].getWave()==m_selectedPath){
						found=true;
						break;
					}
				}
				if(!found){
					done=true;// the selection is not confirmed
				}else{
					m_validationPosition++;// added
					m_Machine_getPaths_DONE=false;
					m_Machine_getPaths_INITIALIZED=false;
				}
			}
		}else if(m_validationPosition>position2){
			m_mappingConfirmed=true;

		}else{
			m_validationPosition++;
			m_Machine_getPaths_DONE=false;
			m_Machine_getPaths_INITIALIZED=false;
		}
	}else{
		// check if it is there for at least overlapMinimumLength
		uint64_t pathId=m_selectedPath;
		int progression=m_selectedPosition;

		// only one path, just go where it goes...
		// except if it has the same number of vertices and
		// the same start and end.
		if(m_FINISH_pathLengths.count(pathId)==0){
			if(!m_FUSION_pathLengthRequested){
				int rankId=getRankFromPathUniqueId(pathId);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));
				message[0]=pathId;
	
				#ifdef ASSERT
				assert(rankId<m_size);
				#endif
	
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rankId,RAY_MPI_TAG_GET_PATH_LENGTH,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_pathLengthRequested=true;
				m_FUSION_pathLengthReceived=false;
			}else if(m_FUSION_pathLengthReceived){
				m_FINISH_pathLengths[pathId]=m_FUSION_receivedLength;
			}
		}else if(m_FINISH_pathLengths[pathId]!=0 // 0 means the path does not exist.
		&&m_FINISH_pathLengths[pathId]!=(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){// avoid fusion of same length.
			int nextPosition=progression+1;
			if(nextPosition<m_FINISH_pathLengths[pathId]){
				// get the vertex
				// get its paths,
				// and continue...
				if(!m_FINISH_vertex_requested){
					int rankId=getRankFromPathUniqueId(pathId);
					uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t)*2);
					message[0]=pathId;
					message[1]=nextPosition;
					Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,rankId,RAY_MPI_TAG_GET_PATH_VERTEX,getRank());
					m_outbox->push_back(aMessage);
					m_FINISH_vertex_requested=true;
					m_FINISH_vertex_received=false;

				}else if(m_FINISH_vertex_received){
					m_FINISH_newFusions[m_FINISH_newFusions.size()-1].push_back(m_FINISH_received_vertex);
					m_FINISH_vertex_requested=false;
					m_selectedPosition++;
					m_FINISH_fusionOccured=true;
				}
			}else{
				#ifdef SHOW_FUSION
				cout<<"Rank "<<getRank()<<": extension-"<<m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i]<<" ("<<m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<<" vertices) and extension-"<<pathId<<" ("<<m_FINISH_pathLengths[pathId]<<" vertices) make a fusion, result: "<<m_FINISH_newFusions[m_FINISH_newFusions.size()-1].size()<<" vertices."<<endl;
				#endif

				done=true;
				if(m_parameters->showMemoryUsage()){
					showMemoryUsage(getRank());
				}
			}
		}else{
			done=true;
		}
	}
	if(done){
		// there is nothing we can do.
		m_seedingData->m_SEEDING_i++;
		m_FINISH_vertex_requested=false;
		m_ed->m_EXTENSION_currentPosition=0;
		m_FUSION_pathLengthRequested=false;
		m_Machine_getPaths_INITIALIZED=false;
		m_Machine_getPaths_DONE=false;
		m_checkedValidity=false;
		delete m_FINISH_pathsForPosition;
		m_FINISH_pathsForPosition=new vector<vector<Direction> >;
		m_FINISH_coverages.clear();
		m_mappingConfirmed=false;
		m_validationPosition=0;
	}
}

void FusionData::makeFusions(){
	// fusion.
	// find a path that matches directly.
	// if a path is 100% included in another, but the other is longer, keep the longest.
	// if a path is 100% identical to another one, keep the one with the lowest ID
	// if a path is 100% identical to another one, but is reverse-complement, keep the one with the lowest ID
	
	int END_LENGTH=100;
	// avoid duplication of contigs.
	if(m_seedingData->m_SEEDING_i<(uint64_t)m_ed->m_EXTENSION_contigs.size()){
		if((int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<=END_LENGTH){
			END_LENGTH=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-1;
		}
	}
	if(m_seedingData->m_SEEDING_i==(uint64_t)m_ed->m_EXTENSION_contigs.size()){

		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_FUSION_DONE,getRank());
		m_outbox->push_back(aMessage);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		int seedIndex=m_seedingData->m_SEEDING_i-1;
		if(m_ed->m_EXTENSION_contigs.size()==0){
			seedIndex++;
		}
		printf("Rank %i is computing fusions [%i/%i] (completed)\n",getRank(),(int)m_ed->m_EXTENSION_contigs.size(),(int)m_ed->m_EXTENSION_contigs.size());
		fflush(stdout);

		m_cacheAllocator.clear();

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_rank);
			now();
		}
		return;
	}else if((int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<=END_LENGTH){
		cout<<"No fusion for me. "<<m_seedingData->m_SEEDING_i<<" "<<m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<<" "<<m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i]<<endl;
		m_FUSION_direct_fusionDone=false;
		m_FUSION_first_done=false;
		m_FUSION_paths_requested=false;
		m_seedingData->m_SEEDING_i++;


		m_Machine_getPaths_INITIALIZED=false;
		m_Machine_getPaths_DONE=false;

		return;
	}else if(!m_FUSION_direct_fusionDone){
		uint64_t currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
		#ifdef ASSERT
		assert(getRankFromPathUniqueId(currentId)<m_size);
		#endif
		if(!m_FUSION_first_done){
			Kmer theVertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][END_LENGTH];
			if(!m_Machine_getPaths_DONE){
				getPaths(theVertex);
			}else{
				if(m_seedingData->m_SEEDING_i%10==0){
					printf("Rank %i is computing fusions [%i/%i]\n",getRank(),(int)m_seedingData->m_SEEDING_i+1,(int)m_ed->m_EXTENSION_contigs.size());
					fflush(stdout);
					if(m_parameters->showMemoryUsage()){
						showMemoryUsage(getRank());
						now();
					}
				}

				m_FUSION_paths_requested=false;
				m_FUSION_firstPaths=m_Machine_getPaths_result;
				
				m_FUSION_first_done=true;
				m_FUSION_last_done=false;
				m_Machine_getPaths_INITIALIZED=false;
				m_Machine_getPaths_DONE=false;
			}
		}else if(!m_FUSION_last_done){
			Kmer theVertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-END_LENGTH];
			if(!m_Machine_getPaths_DONE){
				getPaths(theVertex);
			}else{
				m_FUSION_paths_requested=false;
				m_FUSION_last_done=true;
				m_FUSION_lastPaths=m_Machine_getPaths_result;
				m_FUSION_matches_done=false;
				m_FUSION_matches.clear();
				m_Machine_getPaths_INITIALIZED=false;
				m_Machine_getPaths_DONE=false;
			}
		}else if(!m_FUSION_matches_done){
			m_FUSION_matches_done=true;
			map<uint64_t,int> index;
			map<uint64_t,vector<int> > starts;
			map<uint64_t,vector<int> > ends;

			// extract those that are on both starting and ending vertices.
			for(int i=0;i<(int)m_FUSION_firstPaths.size();i++){
				index[m_FUSION_firstPaths[i].getWave()]++;
				uint64_t pathId=m_FUSION_firstPaths[i].getWave();
				#ifdef ASSERT
				assert(getRankFromPathUniqueId(pathId)<m_size);
				#endif
				int progression=m_FUSION_firstPaths[i].getProgression();
				starts[pathId].push_back(progression);
			}

			for(int i=0;i<(int)m_FUSION_lastPaths.size();i++){
				index[m_FUSION_lastPaths[i].getWave()]++;
				
				uint64_t pathId=m_FUSION_lastPaths[i].getWave();
				#ifdef ASSERT
				assert(getRankFromPathUniqueId(pathId)<m_size);
				#endif
	
				int progression=m_FUSION_lastPaths[i].getProgression();
				ends[pathId].push_back(progression);
			}
			
			for(map<uint64_t,int>::iterator i=index.begin();i!=index.end();++i){
				uint64_t otherPathId=i->first;
				#ifdef ASSERT
				assert(getRankFromPathUniqueId(otherPathId)<m_size);
				#endif
				if(i->second>=2 && otherPathId != currentId){
					// try to find a match with the current size.
					for(int k=0;k<(int)starts[otherPathId].size();k++){
						bool found=false;
						for(int p=0;p<(int)ends[otherPathId].size();p++){
							int observedLength=ends[otherPathId][p]-starts[otherPathId][k]+1;
							int expectedLength=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-2*END_LENGTH+1;
							if(observedLength==expectedLength){
								m_FUSION_matches.push_back(otherPathId);
								found=true;
								break;
							}
						}
						if(found){
							break;
						}
					}
				}
			}
			if(m_FUSION_matches.size()==0){ // no match, go next.
				m_FUSION_direct_fusionDone=true;
				m_FUSION_reverse_fusionDone=false;
				m_FUSION_first_done=false;
				m_FUSION_paths_requested=false;
			}
			m_FUSION_matches_length_done=false;
			m_FUSION_match_index=0;
			m_FUSION_pathLengthRequested=false;
		}else if(!m_FUSION_matches_length_done){
			#ifdef ASSERT
			assert(m_seedingData->m_SEEDING_i<m_ed->m_EXTENSION_identifiers.size());
			#endif
			uint64_t currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
			#ifdef ASSERT
			assert(getRankFromPathUniqueId(currentId)<m_size);
			#endif
			if(m_FUSION_match_index==(int)m_FUSION_matches.size()){// tested all matches, and nothing was found.
				m_FUSION_matches_length_done=true;
			}else if(!m_FUSION_pathLengthRequested){
				#ifdef ASSERT
				assert(m_FUSION_match_index<(int)m_FUSION_matches.size());
				#endif
				uint64_t uniquePathId=m_FUSION_matches[m_FUSION_match_index];
				int rankId=getRankFromPathUniqueId(uniquePathId);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));
				message[0]=uniquePathId;

				#ifdef ASSERT
				if(rankId>=m_size){
					cout<<"UniqueId="<<uniquePathId<<endl;
				}
				assert(rankId<m_size);
				#endif
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rankId,RAY_MPI_TAG_GET_PATH_LENGTH,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_pathLengthRequested=true;
				m_FUSION_pathLengthReceived=false;
			}else if(m_FUSION_pathLengthReceived){
				if(m_FUSION_receivedLength==0){
				}else if(m_FUSION_matches[m_FUSION_match_index]<currentId && m_FUSION_receivedLength == (int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
					m_FUSION_eliminated.insert(currentId);
					m_FUSION_direct_fusionDone=false;
					m_FUSION_first_done=false;
					m_FUSION_paths_requested=false;
					m_seedingData->m_SEEDING_i++;
				}else if(m_FUSION_receivedLength>(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size() ){
					m_FUSION_eliminated.insert(currentId);
					m_FUSION_direct_fusionDone=false;
					m_FUSION_first_done=false;
					m_FUSION_paths_requested=false;
					m_seedingData->m_SEEDING_i++;
				}
				m_FUSION_match_index++;
				m_FUSION_pathLengthRequested=false;
			}
		}else if(m_FUSION_matches_length_done){ // no candidate found for fusion.
			m_FUSION_direct_fusionDone=true;
			m_FUSION_reverse_fusionDone=false;
			m_FUSION_first_done=false;
			m_FUSION_paths_requested=false;
		}
	}else if(!m_FUSION_reverse_fusionDone){
		uint64_t currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];

		if(!m_FUSION_first_done){

			// get the paths going on the first vertex
			int thePosition=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-END_LENGTH;
			#ifdef ASSERT
			assert(m_seedingData->m_SEEDING_i<(uint64_t)m_ed->m_EXTENSION_contigs.size());
			assert(thePosition<(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size());
			#endif
			Kmer theMainVertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][thePosition];
			Kmer theVertex=complementVertex(&theMainVertex,m_wordSize,m_parameters->getColorSpaceMode());

			if(!m_Machine_getPaths_DONE){
				getPaths(theVertex);
			}else{
				m_FUSION_paths_requested=false;
				m_FUSION_firstPaths=m_Machine_getPaths_result;
				m_FUSION_first_done=true;
				m_FUSION_last_done=false;
				m_Machine_getPaths_INITIALIZED=false;
				m_Machine_getPaths_DONE=false;
			}


		}else if(!m_FUSION_last_done){
			// get the paths going on the last vertex.

			Kmer theVertex=complementVertex(&(m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][END_LENGTH]),m_wordSize,m_parameters->getColorSpaceMode());
			if(!m_Machine_getPaths_DONE){
				getPaths(theVertex);
			}else{
				m_FUSION_paths_requested=false;
				m_FUSION_last_done=true;
				m_FUSION_lastPaths=m_Machine_getPaths_result;
				m_FUSION_matches_done=false;
				m_FUSION_matches.clear();
				m_Machine_getPaths_INITIALIZED=false;
				m_Machine_getPaths_DONE=false;
			}

		}else if(!m_FUSION_matches_done){
			m_FUSION_matches_done=true;
			map<uint64_t,int> index;
			map<uint64_t,vector<int> > starts;
			map<uint64_t,vector<int> > ends;
			for(int i=0;i<(int)m_FUSION_firstPaths.size();i++){
				index[m_FUSION_firstPaths[i].getWave()]++;
				uint64_t pathId=m_FUSION_firstPaths[i].getWave();
				int progression=m_FUSION_firstPaths[i].getProgression();
				starts[pathId].push_back(progression);
			}
			for(int i=0;i<(int)m_FUSION_lastPaths.size();i++){
				index[m_FUSION_lastPaths[i].getWave()]++;
				
				uint64_t pathId=m_FUSION_lastPaths[i].getWave();
				int progression=m_FUSION_lastPaths[i].getProgression();
				ends[pathId].push_back(progression);
			}
			for(map<uint64_t,int>::iterator i=index.begin();i!=index.end();++i){
				uint64_t otherPathId=i->first;
				if(i->second>=2 && i->first != currentId){
					// try to find a match with the current size.
					for(int k=0;k<(int)starts[otherPathId].size();k++){
						bool found=false;
						for(int p=0;p<(int)ends[otherPathId].size();p++){
							int observedLength=ends[otherPathId][p]-starts[otherPathId][k]+1;
							int expectedLength=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-2*END_LENGTH+1;
							if(observedLength==expectedLength){
								m_FUSION_matches.push_back(otherPathId);
								found=true;
								break;
							}
						}
						if(found)
							break;
					}
				}
			}
			if(m_FUSION_matches.size()==0){ // no match, go next.
				m_FUSION_direct_fusionDone=false;
				m_FUSION_first_done=false;
				m_FUSION_paths_requested=false;
				m_seedingData->m_SEEDING_i++;
			}
			m_FUSION_matches_length_done=false;
			m_FUSION_match_index=0;
			m_FUSION_pathLengthRequested=false;
		}else if(!m_FUSION_matches_length_done){
			uint64_t currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
			if(m_FUSION_match_index==(int)m_FUSION_matches.size()){
				m_FUSION_matches_length_done=true;
			}else if(!m_FUSION_pathLengthRequested){
				uint64_t uniquePathId=m_FUSION_matches[m_FUSION_match_index];
				int rankId=getRankFromPathUniqueId(uniquePathId);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));
				message[0]=uniquePathId;
				#ifdef ASSERT
				assert(rankId<m_size);
				#endif
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rankId,RAY_MPI_TAG_GET_PATH_LENGTH,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_pathLengthRequested=true;
				m_FUSION_pathLengthReceived=false;
			}else if(m_FUSION_pathLengthReceived){
				// if the length is 0, it means that the contig was not stored because it was too short
				if(m_FUSION_receivedLength==0){
				// we only keep the other one...
				}else if(m_FUSION_matches[m_FUSION_match_index]<currentId && m_FUSION_receivedLength == (int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
					m_FUSION_eliminated.insert(currentId);
					m_FUSION_direct_fusionDone=false;
					m_FUSION_first_done=false;
					m_FUSION_paths_requested=false;
					m_seedingData->m_SEEDING_i++;
				}else if(m_FUSION_receivedLength>(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
					m_FUSION_eliminated.insert(currentId);
					m_FUSION_direct_fusionDone=false;
					m_FUSION_first_done=false;
					m_FUSION_paths_requested=false;
					m_seedingData->m_SEEDING_i++;
				}
				m_FUSION_match_index++;
				m_FUSION_pathLengthRequested=false;
			}
		}else if(m_FUSION_matches_length_done){ // no candidate found for fusion.
			m_FUSION_direct_fusionDone=false;
			m_FUSION_first_done=false;
			m_FUSION_paths_requested=false;
			m_seedingData->m_SEEDING_i++;
		}
	}
}

/*
 * get the Directions taken by a vertex.
 *
 * m_Machine_getPaths_INITIALIZED must be set to false before any calls.
 * also, you must set m_Machine_getPaths_DONE to false;
 *
 * when done, m_Machine_getPaths_DONE is true
 * and
 * the result is in m_Machine_getPaths_result (a vector<Direction>)
 */
void FusionData::getPaths(Kmer vertex){
	if(!m_Machine_getPaths_INITIALIZED){
		m_Machine_getPaths_INITIALIZED=true;
		m_FUSION_paths_requested=false;
		m_Machine_getPaths_DONE=false;
		m_Machine_getPaths_result.clear();
		return;
	}
	if(m_cacheForRepeatedVertices.find(vertex,false)!=NULL){
		SplayNode<Kmer ,Direction*>*node=m_cacheForRepeatedVertices.find(vertex,false);
		#ifdef ASSERT
		assert(node!=NULL);
		#endif
		Direction**ddirect=node->getValue();
		#ifdef ASSERT
		assert(ddirect!=NULL);
		#endif
		Direction*d=*ddirect;
		while(d!=NULL){
			m_Machine_getPaths_result.push_back(*d);
			d=d->getNext();
		}
		m_Machine_getPaths_DONE=true;
	}else if(!m_FUSION_paths_requested){
		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
		int bufferPosition=0;
		vertex.pack(message,&bufferPosition);
		message[bufferPosition++]=0;
		Message aMessage(message,bufferPosition,MPI_UNSIGNED_LONG_LONG,
			m_parameters->_vertexRank(&vertex),RAY_MPI_TAG_ASK_VERTEX_PATHS,getRank());
		m_outbox->push_back(aMessage);
		m_FUSION_paths_requested=true;
		m_FUSION_paths_received=false;
		m_FUSION_receivedPaths.clear();
	}else if(m_FUSION_paths_received){
		#ifdef ASSERT
		for(int i=0;i<(int)m_FUSION_receivedPaths.size();i++){
			assert(getRankFromPathUniqueId(m_FUSION_receivedPaths[i].getWave())<m_size);
		}
		#endif
		// save the result in the cache.
		#ifdef ASSERT
		assert(m_cacheForRepeatedVertices.find(vertex,false)==NULL);
		#endif

		bool inserted;
		SplayNode<Kmer ,Direction*>*node=m_cacheForRepeatedVertices.insert(vertex,&m_cacheAllocator,&inserted);
		int i=0;
		Direction*theDirection=NULL;
		while(i<(int)m_Machine_getPaths_result.size()){
			Direction*newDirection=(Direction*)m_cacheAllocator.allocate(sizeof(Direction)*1);
			*newDirection=m_Machine_getPaths_result[i];
			newDirection->setNext(theDirection);
			theDirection=newDirection;
			i++;
		}

		Direction**ddirect=node->getValue();
		*ddirect=theDirection;

		#ifdef ASSERT
		if(m_Machine_getPaths_result.size()==0){
			assert(*(m_cacheForRepeatedVertices.find(vertex,false)->getValue())==NULL);
		}
		#endif

		m_Machine_getPaths_DONE=true;
	}
}

void FusionData::initialise(){
	m_FUSION_direct_fusionDone=false;
	m_FUSION_first_done=false;
	m_FUSION_paths_requested=false;
	m_timer.constructor();
}
