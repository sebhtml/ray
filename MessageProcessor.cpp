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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include<assert.h>
#include<Read.h>
#include<MessageProcessor.h>
#include<common_functions.h>
#include<ReadAnnotation.h>
#include<SplayTree.h>
#include<Direction.h>
#include<SplayNode.h>
#include<MyForest.h>
#include<SplayTreeIterator.h>
#include<FusionData.h>
#include<Parameters.h>

void MessageProcessor::processMessage(Message*message,
		ExtensionData*ed,
		int*m_numberOfRanksDoneDetectingDistances,
		int*m_numberOfRanksDoneSendingDistances,
		Parameters*parameters,
			int*m_libraryIterator,
			bool*m_libraryIndexInitiated,
			MyForest*m_subgraph,
			MyAllocator*m_outboxAllocator,
				int rank,
			vector<ReadAnnotation>*m_EXTENSION_receivedReads,
			int*m_numberOfMachinesDoneSendingEdges,
			FusionData*m_fusionData,
			vector<vector<VERTEX_TYPE> >*m_EXTENSION_contigs,
			int*m_wordSize,
			int*m_minimumCoverage,
			int*m_seedCoverage,
			int*m_peakCoverage,
			vector<Read*>*m_myReads,
			bool*m_EXTENSION_currentRankIsDone,
	vector<vector<VERTEX_TYPE> >*m_FINISH_newFusions,
		int size,
	MyAllocator*m_inboxAllocator,
	MyAllocator*m_persistentAllocator,
	vector<int>*m_identifiers,
	bool*m_mode_sendDistribution,
	bool*m_alive,
	vector<VERTEX_TYPE>*m_SEEDING_receivedIngoingEdges,
	VERTEX_TYPE*m_SEEDING_receivedKey,
	int*m_SEEDING_i,
	bool*m_colorSpaceMode,
	bool*m_FINISH_fusionOccured,
	bool*m_Machine_getPaths_INITIALIZED,
	int*m_calibration_numberOfMessagesSent,
	int*m_mode,
	vector<vector<VERTEX_TYPE> >*m_allPaths,
	bool*m_EXTENSION_VertexAssembled_received,
	int*m_EXTENSION_numberOfRanksDone,
	int*m_EXTENSION_currentPosition,
	int*m_last_value,
	vector<int>*m_EXTENSION_identifiers,
	int*m_ranksDoneAttachingReads,
	bool*m_SEEDING_edgesReceived,
	PairedRead*m_EXTENSION_pairedRead,
	bool*m_mode_EXTENSION,
	vector<VERTEX_TYPE>*m_SEEDING_receivedOutgoingEdges,
	int*m_DISTRIBUTE_n,
	vector<VERTEX_TYPE>*m_SEEDING_nodes,
	bool*m_EXTENSION_hasPairedReadReceived,
	int*m_numberOfRanksDoneSeeding,
	bool*m_SEEDING_vertexKeyAndCoverageReceived,
	int*m_SEEDING_receivedVertexCoverage,
	bool*m_EXTENSION_readLength_received,
	int*m_calibration_MaxSpeed,
	bool*m_Machine_getPaths_DONE,
	int*m_CLEAR_n,
	bool*m_FINISH_vertex_received,
	bool*m_EXTENSION_initiated,
	int*m_readyToSeed,
	bool*m_SEEDING_NodeInitiated,
	int*m_FINISH_n,
	bool*m_nextReductionOccured,
	bool*m_EXTENSION_hasPairedReadAnswer,
	MyAllocator*m_directionsAllocator,
	map<int,int>*m_FINISH_pathLengths,
	bool*m_EXTENSION_pairedSequenceReceived,
	int*m_EXTENSION_receivedLength,
	int*m_mode_send_coverage_iterator,
	map<int,VERTEX_TYPE>*m_coverageDistribution,
	VERTEX_TYPE*m_FINISH_received_vertex,
	bool*m_EXTENSION_read_vertex_received,
	int*m_sequence_ready_machines,
	bool*m_SEEDING_InedgesReceived,
	bool*m_EXTENSION_vertexIsAssembledResult,
	bool*m_SEEDING_vertexCoverageReceived,
	VERTEX_TYPE*m_EXTENSION_receivedReadVertex,
	int*m_numberOfMachinesReadyForEdgesDistribution,
	int*m_numberOfMachinesReadyToSendDistribution,
	bool*m_mode_send_outgoing_edges,
	int*m_mode_send_edge_sequence_id,
	int*m_mode_send_vertices_sequence_id,
	bool*m_mode_send_vertices,
	int*m_numberOfMachinesDoneSendingVertices,
	int*m_numberOfMachinesDoneSendingCoverage,
	bool*m_EXTENSION_reads_received,
				vector<Message>*m_outbox,
	map<int,int>*m_allIdentifiers,
	OpenAssemblerChooser*m_oa
){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	int tag=message->getTag();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;


	if(tag==TAG_VERTICES_DATA){
		int length=count;
		for(int i=0;i<length;i++){
			VERTEX_TYPE l=incoming[i];

			#ifdef SHOW_PROGRESS
			if((*m_last_value)!=(int)m_subgraph->size() and (int)m_subgraph->size()%100000==0){
				(*m_last_value)=m_subgraph->size();
				cout<<"Rank "<<rank<<" has "<<m_subgraph->size()<<" vertices "<<endl;
			}
			#endif
			SplayNode<VERTEX_TYPE,Vertex>*tmp=m_subgraph->insert(l);
			#ifdef DEBUG
			assert(tmp!=NULL);
			#endif
			if(m_subgraph->inserted()){
				tmp->getValue()->constructor(); 
			}
			tmp->getValue()->setCoverage(tmp->getValue()->getCoverage()+1);
			#ifdef DEBUG
			assert(tmp->getValue()->getCoverage()>0);
			#endif
		}
	}else if(tag==TAG_EXTENSION_DATA_END){
		(*m_EXTENSION_currentRankIsDone)=true;
	}else if(tag==TAG_EXTENSION_START){
		vector<VERTEX_TYPE> a;
		(*m_allPaths).push_back(a);
		int id=incoming[0];
		#ifdef DEBUG
		int rank=id%MAX_NUMBER_OF_MPI_PROCESSES;
		assert(rank<size);
		#endif
		(*m_identifiers).push_back(id);
		(*m_allIdentifiers)[id]=m_identifiers->size()-1;
	}else if(tag==TAG_EXTENSION_END){
	}else if(tag==TAG_START_FUSION){
		(*m_mode)=MODE_FUSION;
		(*m_SEEDING_i)=0;

		m_fusionData->m_FUSION_direct_fusionDone=false;
		m_fusionData->m_FUSION_first_done=false;
		m_fusionData->m_FUSION_paths_requested=false;
	}else if(tag==TAG_BEGIN_CALIBRATION){
		(*m_calibration_numberOfMessagesSent)=0;
		(*m_mode)=MODE_PERFORM_CALIBRATION;
		#ifdef DEBUG

		#endif
	}else if(tag==TAG_ASK_LIBRARY_DISTANCES_FINISHED){
		(*m_numberOfRanksDoneSendingDistances)++;
	}else if(tag==TAG_LIBRARY_DISTANCE){
		for(int i=0;i<count;i+=3){
			parameters->addDistance(incoming[i+0],incoming[i+1],incoming[i+2]);
		}
	}else if(tag==TAG_ASK_LIBRARY_DISTANCES){
		(*m_mode)=MODE_SEND_LIBRARY_DISTANCES;
		(*m_libraryIterator)=0;
		(*m_libraryIndexInitiated)=false;
	}else if(tag==TAG_END_CALIBRATION){
		(*m_mode)=MODE_DO_NOTHING;
		(*m_calibration_MaxSpeed)=(*m_calibration_numberOfMessagesSent)/CALIBRATION_DURATION/size;
		cout<<"Rank "<<rank<<" MaximumSpeed (point-to-point)="<<(*m_calibration_MaxSpeed)<<" messages/second"<<endl;
	}else if(tag==TAG_FINISH_FUSIONS){
		(*m_mode)=MODE_FINISH_FUSIONS;
		(*m_FINISH_fusionOccured)=false;
		(*m_SEEDING_i)=0;
		(*m_EXTENSION_currentPosition)=0;
		m_fusionData->m_FUSION_first_done=false;
		(*m_Machine_getPaths_INITIALIZED)=false;
		(*m_Machine_getPaths_DONE)=false;
	}else if(tag==TAG_COPY_DIRECTIONS){
		(*m_mode)=MODE_COPY_DIRECTIONS;
		for(int i=0;i<m_subgraph->getNumberOfTrees();i++){
			SplayTreeIterator<VERTEX_TYPE,Vertex> seedingIterator(m_subgraph->getTree(i));
			while(seedingIterator.hasNext()){
				SplayNode<VERTEX_TYPE,Vertex>*node=seedingIterator.next();
				(*m_SEEDING_nodes).push_back(node->getKey());
			}
		}
		(*m_SEEDING_i)=0;
	}else if(tag==TAG_EXTENSION_IS_DONE){
		(*m_EXTENSION_numberOfRanksDone)++;
		(*m_EXTENSION_currentRankIsDone)=true;
	}else if(tag==TAG_SET_WORD_SIZE){
		(*m_wordSize)=incoming[0];
	}else if(tag==TAG_SET_COLOR_MODE){
		(*m_colorSpaceMode)=incoming[0];
	}else if(tag==TAG_START_SEEDING){
		(*m_mode)=MODE_START_SEEDING;
		map<int,map<int,int> > edgesDistribution;
		
		for(int i=0;i<m_subgraph->getNumberOfTrees();i++){
			SplayTreeIterator<VERTEX_TYPE,Vertex> seedingIterator(m_subgraph->getTree(i));
			while(seedingIterator.hasNext()){
				SplayNode<VERTEX_TYPE,Vertex>*node=seedingIterator.next();
				edgesDistribution[node->getValue()->getIngoingEdges(node->getKey(),(*m_wordSize)).size()][node->getValue()->getOutgoingEdges(node->getKey(),(*m_wordSize)).size()]++;
				(*m_SEEDING_nodes).push_back(node->getKey());
			}
		}
		#ifdef DEBUG
		//cout<<"Ingoing and outgoing edges."<<endl;
		for(map<int,map<int,int> >::iterator i=edgesDistribution.begin();i!=edgesDistribution.end();++i){
			for(map<int,int>::iterator j=i->second.begin();j!=i->second.end();++j){
				//cout<<i->first<<" "<<j->first<<" "<<j->second<<endl;
			}
		}
		#endif
		(*m_SEEDING_NodeInitiated)=false;
		(*m_SEEDING_i)=0;
	}else if(tag==TAG_FINISH_FUSIONS_FINISHED){
		(*m_FINISH_n)++;
		if(incoming[0]){
			(*m_nextReductionOccured)=incoming[0];
		}
	}else if(tag==TAG_CLEAR_DIRECTIONS){
	
		// clearing old data too!.
		(*m_FINISH_pathLengths).clear();

		// clear graph
		for(int i=0;i<m_subgraph->getNumberOfTrees();i++){
			SplayTreeIterator<VERTEX_TYPE,Vertex> iterator(m_subgraph->getTree(i));
			while(iterator.hasNext()){
				iterator.next()->getValue()->clearDirections();
			}
		}
		(*m_directionsAllocator).clear();
		(*m_directionsAllocator).constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE);


		// add the FINISHING bits
		for(int i=0;i<(int)(*m_FINISH_newFusions).size();i++){
			#ifdef SHOW_PROGRESS
			#endif
			(*m_EXTENSION_contigs).push_back((*m_FINISH_newFusions)[i]);
		}



		(*m_FINISH_newFusions).clear();

		vector<vector<VERTEX_TYPE> > fusions;
		for(int i=0;i<(int)(*m_EXTENSION_contigs).size();i++){
			int id=(*m_EXTENSION_identifiers)[i];
			if(m_fusionData->m_FUSION_eliminated.count(id)==0){
				fusions.push_back((*m_EXTENSION_contigs)[i]);
				vector<VERTEX_TYPE> rc;
				for(int j=(*m_EXTENSION_contigs)[i].size()-1;j>=0;j--){
					rc.push_back(complementVertex((*m_EXTENSION_contigs)[i][j],*m_wordSize,(*m_colorSpaceMode)));
				}
				fusions.push_back(rc);
			}
		}

		(*m_EXTENSION_identifiers).clear();
		m_fusionData->m_FUSION_eliminated.clear();
		for(int i=0;i<(int)fusions.size();i++){
			int id=i*MAX_NUMBER_OF_MPI_PROCESSES+rank;
			#ifdef DEBUG
			assert(id%MAX_NUMBER_OF_MPI_PROCESSES<size);
			#endif
			(*m_EXTENSION_identifiers).push_back(id);
		}

		for(int i=0;i<(int)(*m_EXTENSION_identifiers).size();i++){
			int id=(*m_EXTENSION_identifiers)[i];
			m_fusionData->m_FUSION_identifier_map[id]=i;
		}

		(*m_EXTENSION_contigs).clear();
		(*m_EXTENSION_contigs)=fusions;

		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,TAG_CLEAR_DIRECTIONS_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_CLEAR_DIRECTIONS_REPLY){
		(*m_CLEAR_n)++;
	}else if(tag==TAG_INDEX_PAIRED_SEQUENCE){
		for(int i=0;i<count;i+=5){
			PairedRead*t=(PairedRead*)(*m_persistentAllocator).allocate(sizeof(PairedRead));
			int length=incoming[i+3];
			int deviation=incoming[i+4];

			t->constructor(incoming[i+1],incoming[i+2],length,deviation);
			(*m_myReads)[incoming[i+0]]->setPairedRead(t);
		}
	}else if(tag==TAG_UPDATE_LIBRARY_INFORMATION){
		for(int i=0;i<count;i+=3){
			#ifdef DEBUG
			assert((*m_myReads)[incoming[i+0]]->hasPairedRead());
			#endif
			(*m_myReads)[incoming[i+0]]->getPairedRead()->updateLibrary(incoming[i+1],incoming[i+2]);
		}
	}else if(tag==TAG_COMMUNICATION_STABILITY_MESSAGE){
	}else if(tag==TAG_GET_PATH_VERTEX){
		int id=incoming[0];
		int position=incoming[1];
		#ifdef DEBUG
		assert(m_fusionData->m_FUSION_identifier_map.count(id)>0);
		#endif
		#ifdef DEBUG
		if(position>=(int)(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size()){
			cout<<"Pos="<<position<<" Length="<<(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size()<<endl;
		}
		assert(position<(int)(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size());
		#endif
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(sizeof(VERTEX_TYPE));
		message[0]=(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]][position];
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_GET_PATH_VERTEX_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_GET_PATH_VERTEX_REPLY){
		(*m_FINISH_vertex_received)=true;
		(*m_FINISH_received_vertex)=incoming[0];
	}else if(tag==TAG_GET_PATH_LENGTH){
		int id=incoming[0];
		int length=0;
		#ifdef DEBUG
		assert(m_fusionData->m_FUSION_identifier_map.count(id)>0);
		#endif
		if(m_fusionData->m_FUSION_identifier_map.count(id)>0){
			length=(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size();
		}

		#ifdef DEBUG
		assert(length>0);
		#endif
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(sizeof(VERTEX_TYPE));
		message[0]=length;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_GET_PATH_LENGTH_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_GET_PATH_LENGTH_REPLY){
		m_fusionData->m_FUSION_receivedLength=incoming[0];
		m_fusionData->m_FUSION_pathLengthReceived=true;
	}else if(tag==TAG_REQUEST_VERTEX_COVERAGE){
		SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
		#ifdef DEBUG
		assert(node!=NULL);
		#endif
		VERTEX_TYPE coverage=node->getValue()->getCoverage();
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
		message[0]=coverage;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_COVERAGE_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_DISTRIBUTE_FUSIONS){
		(*m_mode)=MODE_DISTRIBUTE_FUSIONS;
		(*m_SEEDING_i)=0;
		(*m_EXTENSION_currentPosition)=0;
	}else if(tag==TAG_DISTRIBUTE_FUSIONS_FINISHED){
		(*m_DISTRIBUTE_n)++;
	}else if(tag==TAG_HAS_PAIRED_READ){
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
		message[0]=(*m_myReads)[incoming[0]]->hasPairedRead();
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_HAS_PAIRED_READ_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_HAS_PAIRED_READ_REPLY){
		(*m_EXTENSION_hasPairedReadAnswer)=incoming[0];
		(*m_EXTENSION_hasPairedReadReceived)=true;
	}else if(tag==TAG_REQUEST_VERTEX_INGOING_EDGES){
		SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
		#ifdef DEBUG
		assert(node!=NULL);
		#endif
		vector<VERTEX_TYPE> ingoingEdges=node->getValue()->getIngoingEdges(incoming[0],*m_wordSize);
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(ingoingEdges.size()*sizeof(VERTEX_TYPE));
		for(int i=0;i<(int)ingoingEdges.size();i++){
			message[i]=ingoingEdges[i];
		}
		Message aMessage(message,ingoingEdges.size(),MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_CALIBRATION_MESSAGE){
	}else if(tag==TAG_ASK_VERTEX_PATHS_SIZE){
		#ifdef DEBUG
		if(m_subgraph->find(incoming[0])==NULL){
			cout<<"Vertex "<<incoming[0]<<" is not here."<<endl;
		}
		assert(m_subgraph->find(incoming[0])!=NULL);
		#endif
		vector<Direction> paths=m_subgraph->find(incoming[0])->getValue()->getDirections();
		m_fusionData->m_FUSION_cachedDirections[source]=paths;
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
		message[0]=paths.size();
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_VERTEX_PATHS_SIZE_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_ASK_VERTEX_PATHS_SIZE_REPLY){
		m_fusionData->m_FUSION_paths_received=true;
		m_fusionData->m_FUSION_receivedPaths.clear();
		m_fusionData->m_FUSION_numberOfPaths=incoming[0];
	}else if(tag==TAG_ELIMINATE_PATH){
		m_fusionData->m_FUSION_eliminated.insert(incoming[0]);
	}else if(tag==TAG_ASK_VERTEX_PATH){
		int i=incoming[0];
		Direction d=m_fusionData->m_FUSION_cachedDirections[source][i];
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(2*sizeof(VERTEX_TYPE));
		message[0]=d.getWave();
		message[1]=d.getProgression();
		Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_VERTEX_PATH_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_ASK_VERTEX_PATH_REPLY){
		m_fusionData->m_FUSION_path_received=true;
		int pathId=incoming[0];
		int position=incoming[1];
		m_fusionData->m_FUSION_receivedPath.constructor(pathId,position);
	}else if(tag==TAG_ASK_EXTENSION_DATA){
		(*m_mode)=MODE_SEND_EXTENSION_DATA;
		(*m_SEEDING_i)=0;
		(*m_EXTENSION_currentPosition)=0;
	}else if(tag==TAG_GET_PAIRED_READ){
		PairedRead*t=(*m_myReads)[incoming[0]]->getPairedRead();
		#ifdef DEBUG
		assert(t!=NULL);
		#endif
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(4*sizeof(VERTEX_TYPE));
		message[0]=t->getRank();
		message[1]=t->getId();
		message[2]=t->getAverageFragmentLength();
		message[3]=t->getStandardDeviation();
		Message aMessage(message,4,MPI_UNSIGNED_LONG_LONG,source,TAG_GET_PAIRED_READ_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_GET_PAIRED_READ_REPLY){
		(*m_EXTENSION_pairedRead).constructor(incoming[0],incoming[1],incoming[2],incoming[3]);
		(*m_EXTENSION_pairedSequenceReceived)=true;
	}else if(tag==TAG_ASSEMBLE_WAVES){
		(*m_mode)=MODE_ASSEMBLE_WAVES;
		(*m_SEEDING_i)=0;
	}else if(tag==TAG_SAVE_WAVE_PROGRESSION){
		SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
		int wave=incoming[1];
		int progression=incoming[2];
		node->getValue()->addDirection(wave,progression,&(*m_directionsAllocator));
	}else if(tag==TAG_SAVE_WAVE_PROGRESSION_REVERSE){
	}else if(tag==TAG_EXTENSION_DATA){
		(*m_allPaths)[(*m_allPaths).size()-1].push_back(incoming[0]);
	}else if(tag==TAG_FUSION_DONE){
		m_fusionData->m_FUSION_numberOfRanksDone++;
	}else if(tag==TAG_ASK_EXTENSION){
		(*m_EXTENSION_initiated)=false;
		(*m_mode_EXTENSION)=true;
		(*m_last_value)=-1;
	}else if(tag==TAG_ASK_REVERSE_COMPLEMENT){
		SplayNode<VERTEX_TYPE,Vertex>*node=(SplayNode<VERTEX_TYPE,Vertex>*)incoming[0];
		VERTEX_TYPE value=node->getKey();
		VERTEX_TYPE reverseComplement=complementVertex(value,*m_wordSize,(*m_colorSpaceMode));
		int rank=vertexRank(reverseComplement,size);
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(2*sizeof(VERTEX_TYPE));
		Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,rank,TAG_REQUEST_VERTEX_POINTER,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_MARK_AS_ASSEMBLED){
	}else if(tag==TAG_ASK_IS_ASSEMBLED){
		SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
		bool isAssembled=node->getValue()->isAssembled();
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
		message[0]=isAssembled;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_IS_ASSEMBLED_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_ASK_IS_ASSEMBLED_REPLY){
		(*m_EXTENSION_VertexAssembled_received)=true;
		(*m_EXTENSION_vertexIsAssembledResult)=(bool)incoming[0];
	}else if(tag==TAG_REQUEST_VERTEX_OUTGOING_EDGES){
		vector<VERTEX_TYPE> outgoingEdges=m_subgraph->find(incoming[0])->getValue()->getOutgoingEdges(incoming[0],*m_wordSize);
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(outgoingEdges.size()*sizeof(VERTEX_TYPE));
		for(int i=0;i<(int)outgoingEdges.size();i++){
			message[i]=outgoingEdges[i];
		}
		Message aMessage(message,outgoingEdges.size(),MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_GOOD_JOB_SEE_YOU_SOON){
		(*m_alive)=false;
	}else if(tag==TAG_AUTOMATIC_DISTANCE_DETECTION){
		(*m_mode)=MODE_AUTOMATIC_DISTANCE_DETECTION;
		(*m_SEEDING_i)=0;
		(*m_EXTENSION_currentPosition)=0;
		ed->m_EXTENSION_reads_requested=false;
	}else if(tag==TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE){
		(*m_numberOfRanksDoneDetectingDistances)++;
	}else if(tag==TAG_SEEDING_IS_OVER){
		(*m_numberOfRanksDoneSeeding)++;
	}else if(tag==TAG_ATTACH_SEQUENCE){
		for(int i=0;i<count;i+=4){
			VERTEX_TYPE vertex=incoming[i+0];
			int rank=incoming[i+1];
			int sequenceIdOnDestination=(int)incoming[i+2];
			char strand=(char)incoming[i+3];
			#ifdef DEBUG
			assert(m_subgraph->find(vertex)!=NULL);
			#endif
			m_subgraph->find(vertex)->getValue()->addRead(rank,sequenceIdOnDestination,strand,&(*m_persistentAllocator));
		}
	}else if(tag==TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY){
		(*m_SEEDING_receivedIngoingEdges).clear();
		for(int i=0;i<count;i++){
			(*m_SEEDING_receivedIngoingEdges).push_back(incoming[i]);
		}
		(*m_SEEDING_InedgesReceived)=true;
	}else if(tag==TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY){
		(*m_SEEDING_receivedOutgoingEdges).clear();
		for(int i=0;i<count;i++){
			(*m_SEEDING_receivedOutgoingEdges).push_back(incoming[i]);
		}
		(*m_SEEDING_edgesReceived)=true;
	}else if(tag==TAG_ASSEMBLE_WAVES_DONE){
		(*m_EXTENSION_currentRankIsDone)=true;
	}else if(tag==TAG_MASTER_IS_DONE_ATTACHING_READS){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,rank);
		m_outbox->push_back(aMessage);
		#ifdef SHOW_PROGRESS
		#endif
	}else if(tag==TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY){
		(*m_ranksDoneAttachingReads)++;
	}else if(tag==TAG_REQUEST_VERTEX_KEY_AND_COVERAGE){
		SplayNode<VERTEX_TYPE,Vertex>*node=(SplayNode<VERTEX_TYPE,Vertex>*)incoming[0];
		VERTEX_TYPE key=node->getKey();
		VERTEX_TYPE coverage=node->getValue()->getCoverage();
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(2*sizeof(VERTEX_TYPE));
		message[0]=key;
		message[1]=coverage;
		Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY){
		(*m_SEEDING_receivedKey)=incoming[0];
		(*m_SEEDING_receivedVertexCoverage)=incoming[1];
		(*m_SEEDING_vertexKeyAndCoverageReceived)=true;
	}else if(tag==TAG_REQUEST_VERTEX_COVERAGE_REPLY){
		(*m_SEEDING_receivedVertexCoverage)=incoming[0];
		(*m_SEEDING_vertexCoverageReceived)=true;
	}else if(tag==TAG_COVERAGE_DATA){
		int length=count;

		for(int i=0;i<length;i+=2){
			int coverage=incoming[i+0];
			VERTEX_TYPE count=incoming[i+1];
			(*m_coverageDistribution)[coverage]+=count;
		}
	}else if(tag==TAG_SEND_COVERAGE_VALUES){
		(*m_minimumCoverage)=incoming[0];
		(*m_seedCoverage)=incoming[1];
		(*m_peakCoverage)=incoming[2];
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_READY_TO_SEED,rank);
		m_outbox->push_back(aMessage);
		m_oa->constructor((*m_peakCoverage));
	}else if(tag==TAG_READY_TO_SEED){
		(*m_readyToSeed)++;
	}else if(tag==TAG_OUT_EDGES_DATA){
		int length=count;

		for(int i=0;i<(int)length;i+=2){
			VERTEX_TYPE prefix=incoming[i+0];
			VERTEX_TYPE suffix=incoming[i+1];
			#ifdef DEBUG
			assert(m_subgraph->find(prefix)!=NULL);
			#endif
			m_subgraph->find(prefix)->getValue()->addOutgoingEdge(suffix,(*m_wordSize),&(*m_persistentAllocator));
			#ifdef DEBUG
			vector<VERTEX_TYPE> newEdges=m_subgraph->find(prefix)->getValue()->getOutgoingEdges(prefix,(*m_wordSize));
			bool found=false;
			for(int i=0;i<(int)newEdges.size();i++){
				if(newEdges[i]==suffix)
					found=true;
			}
			if(newEdges.size()==0){
				cout<<"prefix,suffix"<<endl;
				coutBIN(prefix);
				coutBIN(suffix);
				cout<<idToWord(prefix,(*m_wordSize))<<endl;
				cout<<idToWord(suffix,(*m_wordSize))<<endl;
			}
			assert(newEdges.size()>0);
			if(!found){
				cout<<"prefix,suffix"<<endl;
				coutBIN(prefix);
				coutBIN(suffix);
				cout<<idToWord(prefix,(*m_wordSize))<<endl;
				cout<<idToWord(suffix,(*m_wordSize))<<endl;

				cout<<" ARC"<<endl;
				coutBIN(prefix);
				coutBIN(suffix);
				cout<<"EDges."<<endl;
				for(int i=0;i<(int)newEdges.size();i++){
					coutBIN(newEdges[i]);
				}

			}
			assert(found);
			#endif
		}
	}else if(tag==TAG_ASK_READ_LENGTH){
		int length=(*m_myReads)[incoming[0]]->length();
		
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
		message[0]=length;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_READ_LENGTH_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_ASK_READ_LENGTH_REPLY){
		(*m_EXTENSION_readLength_received)=true;
		(*m_EXTENSION_receivedLength)=incoming[0];
	}else if(tag==TAG_ASK_READ_VERTEX_AT_POSITION){
		char strand=incoming[2];
		VERTEX_TYPE vertex=(*m_myReads)[incoming[0]]->Vertex(incoming[1],(*m_wordSize),strand,(*m_colorSpaceMode));
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
		message[0]=vertex;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_READ_VERTEX_AT_POSITION_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_ASK_READ_VERTEX_AT_POSITION_REPLY){
		(*m_EXTENSION_read_vertex_received)=true;
		(*m_EXTENSION_receivedReadVertex)=((VERTEX_TYPE*)buffer)[0];
	}else if(tag==TAG_IN_EDGES_DATA){
		int length=count;

		for(int i=0;i<(int)length;i+=2){
			VERTEX_TYPE prefix=incoming[i+0];
			VERTEX_TYPE suffix=incoming[i+1];
			m_subgraph->find(suffix)->getValue()->addIngoingEdge(prefix,(*m_wordSize),&(*m_persistentAllocator));
			#ifdef DEBUG
			bool found=false;
			vector<VERTEX_TYPE> edges=m_subgraph->find(suffix)->getValue()->getIngoingEdges(suffix,(*m_wordSize));
			for(int i=0;i<(int)edges.size();i++){
				if(edges[i]==prefix)
					found=true;
			}
			assert(found);
			#endif
		}
	}else if(tag==TAG_WELCOME){

	}else if(tag==TAG_SEND_SEQUENCE){
		int length=count;
		char*incoming=(char*)(*m_inboxAllocator).allocate(count*sizeof(char)+1);
		for(int i=0;i<(int)length;i++)
			incoming[i]=((char*)buffer)[i];

		incoming[length]='\0';
		Read*myRead=(Read*)(*m_persistentAllocator).allocate(sizeof(Read));
		myRead->copy(NULL,incoming,&(*m_persistentAllocator));
		(*m_myReads).push_back(myRead);
		#ifdef SHOW_PROGRESS
		if((*m_myReads).size()%100000==0){
			cout<<"Rank "<<rank<<" has "<<(*m_myReads).size()<<" sequences"<<endl;
		}
		#endif
	}else if(tag==TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<rank<<" has "<<(*m_myReads).size()<<" sequences"<<endl;
		#endif
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,TAG_SEQUENCES_READY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_SHOW_VERTICES){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<rank<<" has "<<m_subgraph->size()<<" vertices (DONE)"<<endl;
		#endif
		#ifdef SHOW_FOREST_SIZE
		cout<<"Rank "<<rank<<" has "<<m_subgraph->size()<<" vertices (DONE)"<<endl;
		#endif
		m_subgraph->freeze();
	}else if(tag==TAG_SEQUENCES_READY){
		(*m_sequence_ready_machines)++;
	}else if(tag==TAG_START_EDGES_DISTRIBUTION_ANSWER){
		(*m_numberOfMachinesReadyForEdgesDistribution)++;
	}else if(tag==TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER){
		(*m_numberOfMachinesReadyToSendDistribution)++;
	}else if(tag==TAG_PREPARE_COVERAGE_DISTRIBUTION){
		(*m_mode_send_coverage_iterator)=0;
		(*m_mode_sendDistribution)=true;
	}else if(tag==TAG_START_EDGES_DISTRIBUTION_ASK){
		Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, source, TAG_START_EDGES_DISTRIBUTION_ANSWER,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION){
		Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, source, TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_START_EDGES_DISTRIBUTION){
		(*m_mode_send_outgoing_edges)=true;
		(*m_mode_send_edge_sequence_id)=0;
	}else if(tag==TAG_START_VERTICES_DISTRIBUTION){
		(*m_mode_send_vertices)=true;
		(*m_mode_send_vertices_sequence_id)=0;
	}else if(tag==TAG_VERTICES_DISTRIBUTED){
		(*m_numberOfMachinesDoneSendingVertices)++;
	}else if(tag==TAG_COVERAGE_END){
		(*m_numberOfMachinesDoneSendingCoverage)++;
	}else if(tag==TAG_REQUEST_READS){
		vector<ReadAnnotation> reads;
		ReadAnnotation*e=m_subgraph->find(incoming[0])->getValue()->getReads();
		while(e!=NULL){
			reads.push_back(*e);
			e=e->getNext();
		}
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(3*sizeof(VERTEX_TYPE)*reads.size());
		int j=0;
		for(int i=0;i<(int)reads.size();i++){
			message[j++]=reads[i].getRank();
			message[j++]=reads[i].getReadIndex();
			message[j++]=reads[i].getStrand();
		}
		Message aMessage(message,3*reads.size(),MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_READS_REPLY,rank);
		m_outbox->push_back(aMessage);
	}else if(tag==TAG_REQUEST_READS_REPLY){
		m_EXTENSION_receivedReads->clear();
		for(int i=0;i<count;i+=3){
			int rank=incoming[i];
			int index=incoming[i+1];
			char strand=incoming[i+2];
			ReadAnnotation e;
			e.constructor(rank,index,strand);
			m_EXTENSION_receivedReads->push_back(e);
		}
		(*m_EXTENSION_reads_received)=true;
	}else if(tag==TAG_EDGES_DISTRIBUTED){
		(*m_numberOfMachinesDoneSendingEdges)++;
	}else{
		cout<<"Unknown tag"<<tag<<endl;
	}
}
