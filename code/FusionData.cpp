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
#include<FusionData.h>
#include<Message.h>

void FusionData::distribute(SeedingData*m_seedingData,ExtensionData*m_ed,int getRank,RingAllocator*m_outboxAllocator,StaticVector*m_outbox,int getSize1,int*m_mode){
	if(!isReady()){
		return;
	}
	if(!m_buffers.isEmpty() && m_seedingData->m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){
		m_ready+=m_buffers.flushAll(RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY,m_outboxAllocator,m_outbox,getRank);
		return;
	}else if(m_buffers.isEmpty() && m_seedingData->m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){
		printf("Rank %i is distributing fusions [%i/%i] (completed)\n",getRank,(int)m_ed->m_EXTENSION_contigs.size(),(int)m_ed->m_EXTENSION_contigs.size());
		fflush(stdout);
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED,getRank);
		m_outbox->push_back(aMessage);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		return;
	}

	if(m_ed->m_EXTENSION_currentPosition==0){
		if(m_seedingData->m_SEEDING_i%10==0){
			printf("Rank %i is distributing fusions [%i/%i] (completed)\n",getRank,m_seedingData->m_SEEDING_i+1,(int)m_ed->m_EXTENSION_contigs.size());
			fflush(stdout);

		}
	}

	uint64_t vertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
	int destination=vertexRank(vertex,getSize());
	m_buffers.addAt(destination,vertex);
	m_buffers.addAt(destination,m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i]);
	m_buffers.addAt(destination,m_ed->m_EXTENSION_currentPosition);

	if(m_buffers.flush(destination,3,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY,m_outboxAllocator,m_outbox,getRank,false)){
		m_ready++;
	}

	m_ed->m_EXTENSION_currentPosition++;

	// the next one
	if(m_ed->m_EXTENSION_currentPosition==(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
		m_seedingData->m_SEEDING_i++;
		m_ed->m_EXTENSION_currentPosition=0;
	}
}

void FusionData::constructor(int size,int max,int rank,StaticVector*outbox,
		RingAllocator*outboxAllocator,int wordSize,bool colorSpaceMode,
		ExtensionData*ed,SeedingData*seedingData,int*mode){
	m_seedingData=seedingData;
	m_mode=mode;
	m_ed=ed;
	m_buffers.constructor(size,max);
	m_size=size;
	m_rank=rank;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_wordSize=wordSize;
	#ifdef ASSERT
	assert(m_wordSize>0);
	#endif
	m_colorSpaceMode=colorSpaceMode;
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
 * finish hyper fusions now!
 */
void FusionData::finishFusions(){
	if(m_seedingData->m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){
		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
		message[0]=m_FINISH_fusionOccured;
		printf("Rank %i is finishing fusions [%i/%i] (completed)\n",getRank(),(int)m_ed->m_EXTENSION_contigs.size(),(int)m_ed->m_EXTENSION_contigs.size());
		fflush(stdout);
	
		/*
		char number[10];
		sprintf(number,"%d",m_rank);
		string theNumber=number;
		string file="Rank_"+theNumber+".fasta";
		ofstream f(file.c_str());

		for(int i=0;i<(int)m_FINISH_newFusions.size();i++){
			string contig=convertToString(&(m_FINISH_newFusions[i]),m_wordSize);
			f<<">contig-"<<i<<" "<<contig.length()<<" nucleotides"<<endl<<addLineBreaks(contig);
		}
		f.close();
		*/

		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_FINISH_FUSIONS_FINISHED,getRank());
		m_outbox->push_back(aMessage);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		return;
	}
	int overlapMinimumLength=1000;
	if((int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<overlapMinimumLength){
		#ifdef SHOW_PROGRESS
		#endif
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
	
	int currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
	// don't do it if it is removed.

	// start threading the extension
	// as the algorithm advance on it, it stores the path positions.
	// when it reaches a choice, it will use the available path as basis.
	
	// we have the extension in m_ed->m_EXTENSION_contigs[m_SEEDING_i]
	// we get the paths with getPaths
	bool done=false;
	if(m_ed->m_EXTENSION_currentPosition<(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
		if(!m_Machine_getPaths_DONE){
			if(m_ed->m_EXTENSION_currentPosition!=(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-1
			&& m_ed->m_EXTENSION_currentPosition!=(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-overlapMinimumLength){
				m_Machine_getPaths_DONE=true;
			}else{
				getPaths(m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition]);
			}
		}else{
			// remove selfId.
			vector<Direction> a;
			for(int i=0;i<(int)m_Machine_getPaths_result.size();i++){
				if(m_Machine_getPaths_result[i].getWave()!=currentId){
					a.push_back(m_Machine_getPaths_result[i]);
				}
			}
			m_FINISH_pathsForPosition.push_back(a);
			if(m_ed->m_EXTENSION_currentPosition==0){
				if(m_seedingData->m_SEEDING_i%10==0){
					printf("Rank %i is finishing fusions [%i/%i]\n",getRank(),(int)m_seedingData->m_SEEDING_i+1,(int)m_ed->m_EXTENSION_contigs.size());
					fflush(stdout);
				}
				vector<uint64_t> a;
				m_FINISH_newFusions.push_back(a);
				m_FINISH_vertex_requested=false;
				m_FUSION_eliminated.insert(currentId);
				m_FUSION_pathLengthRequested=false;
				m_checkedValidity=false;
			}
			uint64_t vertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
			m_FINISH_newFusions[m_FINISH_newFusions.size()-1].push_back(vertex);
			m_ed->m_EXTENSION_currentPosition++;
			m_Machine_getPaths_DONE=false;
			m_Machine_getPaths_INITIALIZED=false;
		}
	}else if(!m_checkedValidity){
		done=true;
		vector<Direction> directions1=m_FINISH_pathsForPosition[m_FINISH_pathsForPosition.size()-1];
		vector<Direction> directions2=m_FINISH_pathsForPosition[m_FINISH_pathsForPosition.size()-overlapMinimumLength];

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
			map<int,vector<int> > indexOnDirection2;

			set<int> in1;
			
			for(int j=0;j<(int)directions1.size();j++){
				int waveId=directions1[j].getWave();
				in1.insert(waveId);
			}
			//cout<<"Rank "<<getRank()<<" directions1="<<directions1.size()<<" directions2="<<directions2.size()<<endl;

			// index the index for each wave
			for(int j=0;j<(int)directions2.size();j++){
				int waveId=directions2[j].getWave();
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
				int wave1=directions1[i].getWave();
				if(indexOnDirection2.count(wave1)==0){
					continue;
				}
				vector<int> searchResults=indexOnDirection2[wave1];
				int progression1=directions1[i].getProgression();
				for(int j=0;j<(int)searchResults.size();j++){
					int index2=searchResults[j];
					int otherProgression=directions2[index2].getProgression();
					if(progression1-otherProgression+1==overlapMinimumLength){
						// this is 
						done=false;
						hits++;
						m_selectedPath=wave1;
						m_selectedPosition=progression1;
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
	}else{
		// check if it is there for at least overlapMinimumLength
		int pathId=m_selectedPath;
		int progression=m_selectedPosition;

		// only one path, just go where it goes...
		// except if it has the same number of vertices and
		// the same start and end.
		if(m_FINISH_pathLengths.count(pathId)==0){
			if(!m_FUSION_pathLengthRequested){
				int rankId=pathId%MAX_NUMBER_OF_MPI_PROCESSES;
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));
				message[0]=pathId;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rankId,RAY_MPI_TAG_GET_PATH_LENGTH,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_pathLengthRequested=true;
				m_FUSION_pathLengthReceived=false;
			}else if(m_FUSION_pathLengthReceived){
				m_FINISH_pathLengths[pathId]=m_FUSION_receivedLength;
			}
		}else if(m_FINISH_pathLengths[pathId]!=(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){// avoid fusion of same length.
			int nextPosition=progression+1;
			if(nextPosition<m_FINISH_pathLengths[pathId]){
				// get the vertex
				// get its paths,
				// and continue...
				if(!m_FINISH_vertex_requested){
					int rankId=pathId%MAX_NUMBER_OF_MPI_PROCESSES;
					uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t)*2);
					message[0]=pathId;
					message[1]=nextPosition;
					Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,rankId,RAY_MPI_TAG_GET_PATH_VERTEX,getRank());
					m_outbox->push_back(aMessage);
					m_FINISH_vertex_requested=true;
					m_FINISH_vertex_received=false;
				}else if(m_FINISH_vertex_received){
					/*if(!m_Machine_getPaths_DONE){
						getPaths(m_FINISH_received_vertex);
					}else{
					*/
					//m_FINISH_pathsForPosition.push_back(m_Machine_getPaths_result);
					m_FINISH_newFusions[m_FINISH_newFusions.size()-1].push_back(m_FINISH_received_vertex);
					m_FINISH_vertex_requested=false;
					//m_Machine_getPaths_INITIALIZED=false;
					//m_Machine_getPaths_DONE=false;
					m_selectedPosition++;
					m_FINISH_fusionOccured=true;
					//}
				}
			}else{
				#ifdef SHOW_FUSION
				cout<<"Ray says: extension-"<<m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i]<<" ("<<m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<<" vertices) and extension-"<<pathId<<" ("<<m_FINISH_pathLengths[pathId]<<" vertices) make a fusion, result: "<<m_FINISH_newFusions[m_FINISH_newFusions.size()-1].size()<<" vertices."<<endl;
				#endif

				done=true;
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
		m_FINISH_pathsForPosition.clear();
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
	if(m_seedingData->m_SEEDING_i<(int)m_ed->m_EXTENSION_contigs.size()){
		if((int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<=END_LENGTH){
			END_LENGTH=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-1;
		}
	}
	if(m_seedingData->m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){





		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_FUSION_DONE,getRank());
		m_outbox->push_back(aMessage);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		#ifdef SHOW_PROGRESS
		int seedIndex=m_seedingData->m_SEEDING_i-1;
		if(m_ed->m_EXTENSION_contigs.size()==0){
			seedIndex++;
		}
		printf("Rank %i is computing fusions [%i/%i] (completed)\n",getRank(),(int)m_ed->m_EXTENSION_contigs.size(),(int)m_ed->m_EXTENSION_contigs.size());
		fflush(stdout);
		#endif
		#ifdef ASSERT
		//cout<<"Rank "<<getRank()<<" eliminated: "<<m_FUSION_eliminated.size()<<endl;
		#endif
		return;
	}else if((int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<=END_LENGTH){
		#ifdef SHOW_PROGRESS
		cout<<"No fusion for me. "<<m_seedingData->m_SEEDING_i<<" "<<m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()<<" "<<m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i]<<endl;
		#endif
		m_FUSION_direct_fusionDone=false;
		m_FUSION_first_done=false;
		m_FUSION_paths_requested=false;
		m_seedingData->m_SEEDING_i++;
		return;
	}else if(!m_FUSION_direct_fusionDone){
		int currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
		if(!m_FUSION_first_done){
			if(!m_FUSION_paths_requested){
				#ifdef SHOW_PROGRESS
				if(m_seedingData->m_SEEDING_i%10==0){
					printf("Rank %i is computing fusions [%i/%i]\n",getRank(),(int)m_seedingData->m_SEEDING_i+1,(int)m_ed->m_EXTENSION_contigs.size());
					fflush(stdout);
				}
				#endif
				// get the paths going on the first vertex
				#ifdef ASSERT
				assert((int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()>END_LENGTH);
				#endif
				uint64_t theVertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][END_LENGTH];
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
				message[0]=theVertex;

				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex,getSize()),RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_paths_requested=true;
				m_FUSION_paths_received=false;
				m_FUSION_path_id=0;
				m_FUSION_path_requested=false;
			}else if(m_FUSION_paths_received){
				if(m_FUSION_path_id<m_FUSION_numberOfPaths){
					if(!m_FUSION_path_requested){
						uint64_t theVertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][END_LENGTH];
						uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
						message[0]=m_FUSION_path_id;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex,getSize()),RAY_MPI_TAG_ASK_VERTEX_PATH,getRank());
						m_outbox->push_back(aMessage);
						m_FUSION_path_requested=true;
						m_FUSION_path_received=false;
					}else if(m_FUSION_path_received){
						m_FUSION_path_id++;
						m_FUSION_receivedPaths.push_back(m_FUSION_receivedPath);
						m_FUSION_path_requested=false;
					}
				}else{
					m_FUSION_first_done=true;
					m_FUSION_paths_requested=false;
					m_FUSION_last_done=false;
					m_FUSION_firstPaths=m_FUSION_receivedPaths;
					#ifdef ASSERT
					assert(m_FUSION_numberOfPaths==(int)m_FUSION_firstPaths.size());
					#endif
				}
			}
		}else if(!m_FUSION_last_done){
			// get the paths going on the last vertex.

			if(!m_FUSION_paths_requested){
				// get the paths going on the lastvertex<
				#ifdef ASSERT
				assert((int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()>=END_LENGTH);
				#endif
				uint64_t theVertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-END_LENGTH];
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
				message[0]=theVertex;

				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex,getSize()),RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_paths_requested=true;
				m_FUSION_paths_received=false;
				m_FUSION_path_id=0;
				m_FUSION_path_requested=false;
			}else if(m_FUSION_paths_received){
				if(m_FUSION_path_id<m_FUSION_numberOfPaths){
					if(!m_FUSION_path_requested){
						uint64_t theVertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-END_LENGTH];
						uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
						message[0]=m_FUSION_path_id;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex,getSize()),RAY_MPI_TAG_ASK_VERTEX_PATH,getRank());
						m_outbox->push_back(aMessage);
						m_FUSION_path_requested=true;
						m_FUSION_path_received=false;
					}else if(m_FUSION_path_received){
						m_FUSION_path_id++;
						m_FUSION_receivedPaths.push_back(m_FUSION_receivedPath);
						m_FUSION_path_requested=false;
					}
				}else{
					m_FUSION_last_done=true;
					m_FUSION_paths_requested=false;
					m_FUSION_lastPaths=m_FUSION_receivedPaths;
					m_FUSION_matches_done=false;
					m_FUSION_matches.clear();

					#ifdef ASSERT
					assert(m_FUSION_numberOfPaths==(int)m_FUSION_lastPaths.size());
					#endif
				}
			}


		}else if(!m_FUSION_matches_done){
			m_FUSION_matches_done=true;
			map<int,int> index;
			map<int,vector<int> > starts;
			map<int,vector<int> > ends;


			// extract those that are on both starting and ending vertices.
			for(int i=0;i<(int)m_FUSION_firstPaths.size();i++){
				index[m_FUSION_firstPaths[i].getWave()]++;
				int pathId=m_FUSION_firstPaths[i].getWave();
				int progression=m_FUSION_firstPaths[i].getProgression();
				starts[pathId].push_back(progression);
			}

			vector<int> matches;

			for(int i=0;i<(int)m_FUSION_lastPaths.size();i++){
				index[m_FUSION_lastPaths[i].getWave()]++;
				
				int pathId=m_FUSION_lastPaths[i].getWave();
				int progression=m_FUSION_lastPaths[i].getProgression();
				ends[pathId].push_back(progression);
			}
			

			
			for(map<int,int>::iterator i=index.begin();i!=index.end();++i){
				int otherPathId=i->first;
				if(i->second>=2 and otherPathId != currentId){
					// try to find a match with the current size.
					for(int k=0;k<(int)starts[otherPathId].size();k++){
						bool found=false;
						for(int p=0;p<(int)ends[otherPathId].size();p++){
							int observedLength=ends[otherPathId][p]-starts[otherPathId][k]+1;
							int expectedLength=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-2*END_LENGTH+1;
							//cout<<observedLength<<" versus "<<expectedLength<<endl;
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
				m_FUSION_direct_fusionDone=true;
				m_FUSION_reverse_fusionDone=false;
				m_FUSION_first_done=false;
				m_FUSION_paths_requested=false;
			}
			m_FUSION_matches_length_done=false;
			m_FUSION_match_index=0;
			m_FUSION_pathLengthRequested=false;
		}else if(!m_FUSION_matches_length_done){
			int currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
			if(m_FUSION_match_index==(int)m_FUSION_matches.size()){// tested all matches, and nothing was found.
				m_FUSION_matches_length_done=true;
			}else if(!m_FUSION_pathLengthRequested){
				int uniquePathId=m_FUSION_matches[m_FUSION_match_index];
				int rankId=uniquePathId%MAX_NUMBER_OF_MPI_PROCESSES;
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));
				message[0]=uniquePathId;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rankId,RAY_MPI_TAG_GET_PATH_LENGTH,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_pathLengthRequested=true;
				m_FUSION_pathLengthReceived=false;
			}else if(m_FUSION_pathLengthReceived){
				if(m_FUSION_receivedLength==0){
				}else if(m_FUSION_matches[m_FUSION_match_index]<currentId and m_FUSION_receivedLength == (int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
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
		int currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
		if(!m_FUSION_first_done){
			if(!m_FUSION_paths_requested){
				// get the paths going on the first vertex
				int thePosition=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-END_LENGTH;
				#ifdef ASSERT
				assert(m_seedingData->m_SEEDING_i<(int)m_ed->m_EXTENSION_contigs.size());
				assert(thePosition<(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size());
				#endif
				uint64_t theMainVertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][thePosition];
				uint64_t theVertex=complementVertex(theMainVertex,m_wordSize,m_colorSpaceMode);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
				message[0]=theVertex;

				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex,getSize()),RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_paths_requested=true;
				m_FUSION_paths_received=false;
				m_FUSION_path_id=0;
				m_FUSION_path_requested=false;
			}else if(m_FUSION_paths_received){
				if(m_FUSION_path_id<m_FUSION_numberOfPaths){
					if(!m_FUSION_path_requested){
						uint64_t theVertex=complementVertex(m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-END_LENGTH],m_wordSize,m_colorSpaceMode);
						uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
						message[0]=m_FUSION_path_id;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex,getSize()),RAY_MPI_TAG_ASK_VERTEX_PATH,getRank());
						m_outbox->push_back(aMessage);
						m_FUSION_path_requested=true;
						m_FUSION_path_received=false;
					}else if(m_FUSION_path_received){
						m_FUSION_path_id++;
						m_FUSION_receivedPaths.push_back(m_FUSION_receivedPath);
						m_FUSION_path_requested=false;
					}
				}else{
					m_FUSION_first_done=true;
					m_FUSION_paths_requested=false;
					m_FUSION_last_done=false;
					m_FUSION_firstPaths=m_FUSION_receivedPaths;
					#ifdef ASSERT
					assert(m_FUSION_numberOfPaths==(int)m_FUSION_firstPaths.size());
					#endif
				}
			}
		}else if(!m_FUSION_last_done){
			// get the paths going on the last vertex.

			if(!m_FUSION_paths_requested){
				// get the paths going on the first vertex
				uint64_t theVertex=complementVertex(m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][END_LENGTH],m_wordSize,m_colorSpaceMode);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
				message[0]=theVertex;
				int destination=vertexRank(theVertex,getSize());

				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,destination,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_paths_requested=true;
				m_FUSION_paths_received=false;
				m_FUSION_path_id=0;
				m_FUSION_path_requested=false;
			}else if(m_FUSION_paths_received){
				if(m_FUSION_path_id<m_FUSION_numberOfPaths){
					if(!m_FUSION_path_requested){
						uint64_t theVertex=complementVertex(m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][END_LENGTH],m_wordSize,m_colorSpaceMode);
						uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
						message[0]=m_FUSION_path_id;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex,getSize()),RAY_MPI_TAG_ASK_VERTEX_PATH,getRank());
						m_outbox->push_back(aMessage);
						m_FUSION_path_requested=true;
						m_FUSION_path_received=false;
					}else if(m_FUSION_path_received){
						m_FUSION_path_id++;
						m_FUSION_receivedPaths.push_back(m_FUSION_receivedPath);
						m_FUSION_path_requested=false;
					}
				}else{
					m_FUSION_last_done=true;
					m_FUSION_paths_requested=false;
					m_FUSION_lastPaths=m_FUSION_receivedPaths;
					m_FUSION_matches_done=false;
					m_FUSION_matches.clear();

					#ifdef ASSERT
					assert(m_FUSION_numberOfPaths==(int)m_FUSION_lastPaths.size());
					#endif
				}
			}



		}else if(!m_FUSION_matches_done){
			m_FUSION_matches_done=true;
			map<int,int> index;
			map<int,vector<int> > starts;
			map<int,vector<int> > ends;
			for(int i=0;i<(int)m_FUSION_firstPaths.size();i++){
				index[m_FUSION_firstPaths[i].getWave()]++;
				int pathId=m_FUSION_firstPaths[i].getWave();
				int progression=m_FUSION_firstPaths[i].getProgression();
				starts[pathId].push_back(progression);
			}
			for(int i=0;i<(int)m_FUSION_lastPaths.size();i++){
				index[m_FUSION_lastPaths[i].getWave()]++;
				
				int pathId=m_FUSION_lastPaths[i].getWave();
				int progression=m_FUSION_lastPaths[i].getProgression();
				ends[pathId].push_back(progression);
			}
			vector<int> matches;
			for(map<int,int>::iterator i=index.begin();i!=index.end();++i){
				int otherPathId=i->first;
				if(i->second>=2 and i->first != currentId){
					// try to find a match with the current size.
					for(int k=0;k<(int)starts[otherPathId].size();k++){
						bool found=false;
						for(int p=0;p<(int)ends[otherPathId].size();p++){
							int observedLength=ends[otherPathId][p]-starts[otherPathId][k]+1;
							int expectedLength=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()-2*END_LENGTH+1;
							//cout<<observedLength<<" versus "<<expectedLength<<endl;
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
			int currentId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
			if(m_FUSION_match_index==(int)m_FUSION_matches.size()){
				m_FUSION_matches_length_done=true;
			}else if(!m_FUSION_pathLengthRequested){
				int uniquePathId=m_FUSION_matches[m_FUSION_match_index];
				int rankId=uniquePathId%MAX_NUMBER_OF_MPI_PROCESSES;
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));
				message[0]=uniquePathId;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rankId,RAY_MPI_TAG_GET_PATH_LENGTH,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_pathLengthRequested=true;
				m_FUSION_pathLengthReceived=false;
			}else if(m_FUSION_pathLengthReceived){
				if(m_FUSION_receivedLength==0){
				}else if(m_FUSION_matches[m_FUSION_match_index]<currentId and m_FUSION_receivedLength == (int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
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
void FusionData::getPaths(uint64_t vertex){
	if(!m_Machine_getPaths_INITIALIZED){
		m_Machine_getPaths_INITIALIZED=true;
		m_FUSION_paths_requested=false;
		m_Machine_getPaths_DONE=false;
		m_Machine_getPaths_result.clear();
		return;
	}

	if(!m_FUSION_paths_requested){
		uint64_t theVertex=vertex;
		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
		message[0]=theVertex;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex,getSize()),RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,getRank());
		m_outbox->push_back(aMessage);
		m_FUSION_paths_requested=true;
		m_FUSION_paths_received=false;
		m_FUSION_path_id=0;
		m_FUSION_path_requested=false;
		m_FUSION_receivedPaths.clear();
	}else if(m_FUSION_paths_received){
		if(m_FUSION_path_id<m_FUSION_numberOfPaths){
			if(!m_FUSION_path_requested){
				uint64_t theVertex=vertex;
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
				message[0]=m_FUSION_path_id;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex,getSize()),RAY_MPI_TAG_ASK_VERTEX_PATH,getRank());
				m_outbox->push_back(aMessage);
				m_FUSION_path_requested=true;
				m_FUSION_path_received=false;
			}else if(m_FUSION_path_received){
				m_FUSION_path_id++;
				m_Machine_getPaths_result.push_back(m_FUSION_receivedPath);
				m_FUSION_path_requested=false;
			}
		}else{
			m_Machine_getPaths_DONE=true;
		}
	}
}

