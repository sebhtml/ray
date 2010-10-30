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


#include<Machine.h>
#include<VerticesExtractor.h>
#include<sstream>
#include<Message.h>
#include<time.h>
#include<RepeatedVertexWatchdog.h>
#include<TipWatchdog.h>
#include<BubbleTool.h>
#include<assert.h>
#include<common_functions.h>
#include<iostream>
#include<fstream>
#include<CoverageDistribution.h>
#include<string.h>
#include<SplayTreeIterator.h>
#include<mpi.h>
#include<Read.h>
#include<Loader.h>
#include<MyAllocator.h>
#include<unistd.h>


using namespace std;

void Machine::showUsage(){
	cout<<endl;
	cout<<"Supported sequences file format: "<<endl;

	cout<<".fasta, .fastq, .sff"<<endl;

	cout<<endl;

	cout<<"Parameters:"<<endl;
	cout<<endl;

    	cout<<" -s <sequencesFile> "<<endl;
	cout<<endl;
	cout<<" -p <leftSequencesFile> <rightSequencesFile> [ <fragmentLength> <standardDeviation> ]"<<endl;
	cout<<endl;
	cout<<" -o <outputFile> (default: Ray-Contigs.fasta)"<<endl;
	cout<<endl;
	cout<<" -a [ <amosFile> ] (default: Ray-Contigs.afg)"<<endl;
    	cout<<endl;
	cout<<" -k <kmerSize> (default: 21)"<<endl;
	cout<<endl;

	cout<<endl;
	cout<<"use --help to show this help"<<endl;
	cout<<endl;
}

void Machine::sendLibraryDistances(){
	if(m_libraryIterator==(int)m_libraryDistances.size()){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_ASK_LIBRARY_DISTANCES_FINISHED,getRank());
		m_outbox.push_back(aMessage);
		m_mode=MODE_DO_NOTHING;
	}else if(m_libraryIndex==m_libraryDistances[m_libraryIterator].end()){
		m_libraryIterator++;
		m_libraryIndexInitiated=false;
	}else{
		if(!m_libraryIndexInitiated){
			m_libraryIndexInitiated=true;
			m_libraryIndex=m_libraryDistances[m_libraryIterator].begin();
		}
		int library=m_libraryIterator;
		int distance=m_libraryIndex->first;
		int count=m_libraryIndex->second;
		u64*message=(u64*)m_outboxAllocator.allocate(3*sizeof(u64));
		message[0]=library;
		message[1]=distance;
		message[2]=count;
		Message aMessage(message,3,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_LIBRARY_DISTANCE,getRank());
		m_outbox.push_back(aMessage);
		m_libraryIndex++;
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
void Machine::getPaths(VERTEX_TYPE vertex){
	if(!m_Machine_getPaths_INITIALIZED){
		m_Machine_getPaths_INITIALIZED=true;
		m_fusionData->m_FUSION_paths_requested=false;
		m_Machine_getPaths_DONE=false;
		m_Machine_getPaths_result.clear();
		return;
	}

	if(!m_fusionData->m_FUSION_paths_requested){
		VERTEX_TYPE theVertex=vertex;
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
		message[0]=theVertex;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATHS_SIZE,getRank());
		m_outbox.push_back(aMessage);
		m_fusionData->m_FUSION_paths_requested=true;
		m_fusionData->m_FUSION_paths_received=false;
		m_fusionData->m_FUSION_path_id=0;
		m_fusionData->m_FUSION_path_requested=false;
		m_fusionData->m_FUSION_receivedPaths.clear();
	}else if(m_fusionData->m_FUSION_paths_received){
		if(m_fusionData->m_FUSION_path_id<m_fusionData->m_FUSION_numberOfPaths){
			if(!m_fusionData->m_FUSION_path_requested){
				VERTEX_TYPE theVertex=vertex;
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=m_fusionData->m_FUSION_path_id;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATH,getRank());
				m_outbox.push_back(aMessage);
				m_fusionData->m_FUSION_path_requested=true;
				m_fusionData->m_FUSION_path_received=false;
			}else if(m_fusionData->m_FUSION_path_received){
				m_fusionData->m_FUSION_path_id++;
				m_Machine_getPaths_result.push_back(m_fusionData->m_FUSION_receivedPath);
				m_fusionData->m_FUSION_path_requested=false;
			}
		}else{
			m_Machine_getPaths_DONE=true;
		}
	}
}

Machine::Machine(int argc,char**argv){
	m_argc=argc;
	m_argv=argv;
	m_bubbleData=new BubbleData();
	#ifdef SHOW_SENT_MESSAGES
	m_stats=new StatisticsData();
	#endif
	m_dfsData=new DepthFirstSearchData();
	m_fusionData=new FusionData();
	m_disData=new DistributionData();
	m_ed=new ExtensionData();
	m_sd=new ScaffolderData();
	m_cd=new ChooserData();
}

void Machine::flushIngoingEdges(int threshold){
	// send messages
	for(int rankId=0;rankId<getSize();rankId++){
		int destination=rankId;
		int length=m_disData->m_messagesStockIn.size(rankId);
		if(length<threshold)
			continue;
		VERTEX_TYPE*data=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*(length));
		for(int j=0;j<(int)length;j++){
			data[j]=m_disData->m_messagesStockIn.getAt(rankId,j);
		}
		m_disData->m_messagesStockIn.reset(rankId);

		Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, TAG_IN_EDGES_DATA,getRank());
		m_outbox.push_back(aMessage);
	}
}

void Machine::flushOutgoingEdges(int threshold){
	for(int rankId=0;rankId<(int)getSize();rankId++){
		int destination=rankId;
		int length=m_disData->m_messagesStockOut.size(rankId);
		if(length<threshold)
			continue;
		#ifdef SHOW_PROGRESS
		#endif
		VERTEX_TYPE*data=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*(length));
		for(int j=0;j<(int)length;j++){
			data[j]=m_disData->m_messagesStockOut.getAt(rankId,j);
		}
		m_disData->m_messagesStockOut.reset(rankId);
		Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, TAG_OUT_EDGES_DATA,getRank());
		m_outbox.push_back(aMessage);
	}
}

void Machine::start(){
	m_repeatedLength=0;
	m_maxCoverage=0;
	m_maxCoverage--;// underflow.

	int numberOfTrees=_FOREST_SIZE;

	#ifdef SHOW_PROGRESS
	#endif
	m_startingTime=time(NULL);
	m_lastTime=time(NULL);
	srand(m_lastTime);
	m_fusionData->m_fusionStarted=false;
	m_ed->m_EXTENSION_numberOfRanksDone=0;
	m_colorSpaceMode=false;
	m_messageSentForEdgesDistribution=false;
	m_numberOfRanksDoneSeeding=0;
	m_calibrationAskedCalibration=false;
	m_calibrationIsDone=true; // set to false to perform a speed calibration.
	m_master_mode=MODE_DO_NOTHING;
	m_numberOfMachinesReadyForEdgesDistribution=0;
	m_ed->m_mode_EXTENSION=false;
	m_aborted=false;
	m_readyToSeed=0;
	m_wordSize=-1;
	m_reverseComplementVertex=false;
	m_last_value=0;
	m_mode_send_ingoing_edges=false;
	m_mode_send_edge_sequence_id_position=0;
	m_mode_send_vertices=false;
	m_mode_sendDistribution=false;
	m_mode_send_outgoing_edges=false;
	m_mode_send_vertices_sequence_id=0;
	m_mode_send_vertices_sequence_id_position=0;
	m_reverseComplementEdge=false;
	m_calibration_MaxSpeed=99999999; // initial speed limit before calibration
	m_numberOfMachinesDoneSendingVertices=0;
	m_numberOfMachinesDoneSendingEdges=0;
	m_numberOfMachinesReadyToSendDistribution=0;
	m_numberOfMachinesDoneSendingCoverage=0;
	m_machineRank=0;
	m_messageSentForVerticesDistribution=false;
	m_sequence_ready_machines=0;
	m_isFinalFusion=false;

	m_outboxAllocator.constructor(OUTBOX_ALLOCATOR_CHUNK_SIZE);
	m_inboxAllocator.constructor(INBOX_ALLOCATOR_CHUNK_SIZE);
	m_distributionAllocator.constructor(DISTRIBUTION_ALLOCATOR_CHUNK_SIZE);
	m_persistentAllocator.constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE);
	m_directionsAllocator.constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE);


	m_mode=MODE_DO_NOTHING;
	m_mode_AttachSequences=false;
	m_startEdgeDistribution=false;

	m_ranksDoneAttachingReads=0;

	char serverName[1000];
	int len;
	MPI_Init(&m_argc,&m_argv);
	MPI_Get_processor_name(serverName,&len);

	MPI_Comm_rank(MPI_COMM_WORLD,&m_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&m_size);
	m_disData->constructor(getSize(),5000,&m_persistentAllocator);

	m_subgraph.constructor(numberOfTrees,&m_persistentAllocator);
	
	int version;
	int subversion;
	MPI_Get_version(&version,&subversion);

	if(isMaster()){
		cout<<"**************************************************"<<endl;
    		cout<<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
    		cout<<"This is free software, and you are welcome to redistribute it"<<endl;
    		cout<<"under certain conditions; see \"gpl-3.0.txt\" for details."<<endl;
		cout<<"**************************************************"<<endl;
		cout<<endl;
		cout<<"Ray Copyright (C) 2010  Sébastien Boisvert, Jacques Corbeil, François Laviolette"<<endl;
 		cout<<"http://denovoassembler.sf.net/"<<endl<<endl;

		cout<<"Reference to cite: "<<endl<<endl;
		cout<<"Ray: simultaneous assembly of reads from a mix of high-throughput sequencing technologies."<<endl;
		cout<<"Sébastien Boisvert, François Laviolette, and Jacques Corbeil."<<endl;
		cout<<"Journal of Computational Biology (Mary Ann Liebert, Inc. publishers)."<<endl;
		cout<<"-Not available-, ahead of print."<<endl;
		cout<<"doi:10.1089/cmb.2009.0238"<<endl;
		cout<<"http://dx.doi.org/doi:10.1089/cmb.2009.0238"<<endl;
		cout<<endl;


		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<" welcomes you to the MPI_COMM_WORLD."<<endl;
		cout<<"Rank "<<getRank()<<": website -> http://denovoassembler.sf.net/"<<endl;
		#ifdef MPICH2_VERSION
		cout<<"Rank "<<getRank()<<": using MPICH2"<<endl;
		#else
			#ifdef OMPI_MPI_H
			cout<<"Rank "<<getRank()<<": using Open-MPI "<<OMPI_MAJOR_VERSION<<"."<<OMPI_MINOR_VERSION<<"."<<OMPI_RELEASE_VERSION<<endl;

			#else
			cout<<"Rank "<<getRank()<<": Warning, unknown implementation of MPI"<<endl;
			#endif
		#endif
		#endif
	}

	cout<<"Rank "<<getRank()<<" is running as UNIX process "<<getpid()<<" on "<<serverName<<" (MPI version "<<version<<"."<<subversion<<")"<<endl;
	m_alive=true;
	m_welcomeStep=true;
	m_loadSequenceStep=false;
	m_totalLetters=0;
	m_distribution_file_id=m_distribution_sequence_id=m_distribution_currentSequenceId=0;

	MPI_Barrier(MPI_COMM_WORLD);

	if(m_argc==1 or ((string)m_argv[1])=="--help"){
		if(isMaster()){
			showUsage();
		}
	}else{
		run();
	}

	MPI_Barrier(MPI_COMM_WORLD);

	if(isMaster()){
		computeTime(m_startingTime);
	}
	MPI_Finalize();
}

/*
 * this is the function that runs a lots
 *
 * it
 * 	1) receives messages
 * 	2) free Request if any (only with MPICH2, Open-MPI is better designed and send small messages more efficiently!)
 * 	3) process message. The function that deals with a message is selected with the message's tag
 * 	4) process data, this depends on the master-mode and slave-mode states.
 * 	5) send messages
 */
void Machine::run(){
	#ifdef SHOW_PROGRESS
	if(isMaster()){
		cout<<"Rank "<<getRank()<<": I am the master among "<<getSize()<<" ranks in the MPI_COMM_WORLD."<<endl;
	}
	#endif

	if(isMaster()){
	}
	while(isAlive()){
		receiveMessages(); 
		#ifdef MPICH2_VERSION
		checkRequests();
		#endif
		processMessages();
		processData();
		sendMessages();
	}
}

/*
 * free the memory of Requests
 */
void Machine::checkRequests(){
	MPI_Request theRequests[1024];
	MPI_Status theStatus[1024];
	
	for(int i=0;i<(int)m_pendingMpiRequest.size();i++){
		theRequests[i]=m_pendingMpiRequest[i];
	}
	MPI_Waitall(m_pendingMpiRequest.size(),theRequests,theStatus);
	m_pendingMpiRequest.clear();
}

/*
 * send messages,
 * if the message goes to self, do a memcpy!
 */
void Machine::sendMessages(){
	for(int i=0;i<(int)m_outbox.size();i++){
		Message*aMessage=&(m_outbox[i]);
		int sizeOfElements=8;
		if(aMessage->getTag()==TAG_SEND_SEQUENCE){
			sizeOfElements=1;
		}

		#ifdef SHOW_SENT_MESSAGES
		m_stats->m_statistics_messages[aMessage->getDestination()]++;
		int bytes=sizeOfElements*aMessage->getCount();
		m_stats->m_statistics_bytes[aMessage->getDestination()]+=bytes;
		#endif


		if(aMessage->getDestination()==getRank()){
			void*newBuffer=m_inboxAllocator.allocate(sizeOfElements*aMessage->getCount());
			memcpy(newBuffer,aMessage->getBuffer(),sizeOfElements*aMessage->getCount());
			aMessage->setBuffer(newBuffer);
			m_inbox.push_back(*aMessage);
			continue;
		}

		#ifdef MPICH2_VERSION // MPICH2 waits for the response on the other end.
		MPI_Request request;
		MPI_Status status;
		int flag;
		MPI_Isend(aMessage->getBuffer(), aMessage->getCount(), aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(), MPI_COMM_WORLD,&request);
		MPI_Test(&request,&flag,&status);
		if(!flag){
			m_pendingMpiRequest.push_back(request);
		}
		#else // Open-MPI-1.4.1 sends message eagerly, which is just a better design.
		MPI_Send(aMessage->getBuffer(), aMessage->getCount(), aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(), MPI_COMM_WORLD);
		#endif
	}

	m_outbox.clear();
	m_outboxAllocator.reset();
	#ifdef SHOW_SENT_MESSAGES
	time_t tmp=time(NULL);
	if(tmp>m_stats->m_time_t_statistics){
		m_stats->m_time_t_statistics=tmp;
		cout<<"Time="<<tmp<<" Source="<<getRank()<<" ";
		for(int i=0;i<getSize();i++){
			int v=m_stats->m_statistics_bytes[i];
			string units="B";
			if(v>=1024){
				v/=1024;
				units="kiB";
			}
			
			cout<<" "<<m_stats->m_statistics_messages[i]<<"("<<v<<units<<")";
		}
		cout<<endl;

		m_stats->m_statistics_bytes.clear();
		m_stats->m_statistics_messages.clear();
	}
	#endif
}

/*	
 * using Iprobe, probe for new messages as they arrive
 * if no more message are available, return.
 * messages are kept in the inbox.
 */

void Machine::receiveMessages(){
	int flag;
	MPI_Status status;
	MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	int read=0;
	while(flag){
		read++;
		MPI_Datatype datatype=MPI_UNSIGNED_LONG_LONG;
		int sizeOfType=8;
		int tag=status.MPI_TAG;
		if(tag==TAG_SEND_SEQUENCE){
			datatype=MPI_BYTE;
			sizeOfType=1;
		}
		int source=status.MPI_SOURCE;
		int length;
		MPI_Get_count(&status,datatype,&length);
		void*incoming=(void*)m_inboxAllocator.allocate(length*sizeOfType);
		MPI_Status status2;
		MPI_Recv(incoming,length,datatype,source,tag,MPI_COMM_WORLD,&status2);
		Message aMessage(incoming,length,datatype,source,tag,source);
		m_inbox.push_back(aMessage);
		MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	}

}

int Machine::getRank(){
	return m_rank;
}





/*
 * finish hyper fusions now!
 */
void Machine::finishFusions(){
	// finishing is broken?
	if(m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
		message[0]=m_FINISH_fusionOccured;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_FINISH_FUSIONS_FINISHED,getRank());
		m_outbox.push_back(aMessage);
		m_mode=MODE_DO_NOTHING;
		return;
	}
	int overlapMinimumLength=1000;
	if((int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()<overlapMinimumLength){
		#ifdef SHOW_PROGRESS
		#endif
		m_SEEDING_i++;
		m_FINISH_vertex_requested=false;
		m_ed->m_EXTENSION_currentPosition=0;
		m_fusionData->m_FUSION_pathLengthRequested=false;
		m_Machine_getPaths_INITIALIZED=false;
		m_Machine_getPaths_DONE=false;
		m_checkedValidity=false;
		return;
	}
	// check if the path begins with someone else.
	
	int currentId=m_ed->m_EXTENSION_identifiers[m_SEEDING_i];
	// don't do it if it is removed.

	// start threading the extension
	// as the algorithm advance on it, it stores the path positions.
	// when it reaches a choice, it will use the available path as basis.
	
	// we have the extension in m_ed->m_EXTENSION_contigs[m_SEEDING_i]
	// we get the paths with getPaths
	bool done=false;
	if(m_ed->m_EXTENSION_currentPosition<(int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()){
		if(!m_Machine_getPaths_DONE){
			getPaths(m_ed->m_EXTENSION_contigs[m_SEEDING_i][m_ed->m_EXTENSION_currentPosition]);
		}else{
			vector<Direction> a;
			for(int i=0;i<(int)m_Machine_getPaths_result.size();i++){
				if(m_Machine_getPaths_result[i].getWave()!=currentId){
					a.push_back(m_Machine_getPaths_result[i]);
				}
			}
			m_FINISH_pathsForPosition.push_back(a);
			if(m_ed->m_EXTENSION_currentPosition==0){
				vector<VERTEX_TYPE> a;
				m_FINISH_newFusions.push_back(a);
				m_FINISH_vertex_requested=false;
				m_fusionData->m_FUSION_eliminated.insert(currentId);
				m_fusionData->m_FUSION_pathLengthRequested=false;
				m_checkedValidity=false;
			}
			VERTEX_TYPE vertex=m_ed->m_EXTENSION_contigs[m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
			m_FINISH_newFusions[m_FINISH_newFusions.size()-1].push_back(vertex);
			m_ed->m_EXTENSION_currentPosition++;
			m_Machine_getPaths_DONE=false;
			m_Machine_getPaths_INITIALIZED=false;
		}
	}else if(!m_checkedValidity){
		done=true;
		vector<Direction> directions1=m_FINISH_pathsForPosition[m_FINISH_pathsForPosition.size()-1];
		vector<Direction> directions2=m_FINISH_pathsForPosition[m_FINISH_pathsForPosition.size()-overlapMinimumLength];
		int hits=0;
		for(int i=0;i<(int)directions1.size();i++){
			for(int j=0;j<(int)directions2.size();j++){
				if(directions1[i].getWave()==directions2[j].getWave()){
					int progression=directions1[i].getProgression();
					int otherProgression=directions2[j].getProgression();
					if(progression-otherProgression+1==overlapMinimumLength){
						// this is 
						done=false;
						hits++;
						m_selectedPath=directions1[i].getWave();
						m_selectedPosition=directions1[i].getProgression();
					}
				}
			}
		}
		if(hits>1){// we don't support that right now.
			done=true;
		}
		m_checkedValidity=true;
	}else{
		// check if it is there for at least overlapMinimumLength
		int pathId=m_selectedPath;
		int progression=m_selectedPosition;

		// only one path, just go where it goes...
		// except if it has the same number of vertices and
		// the same start and end.
		if(m_FINISH_pathLengths.count(pathId)==0){
			if(!m_fusionData->m_FUSION_pathLengthRequested){
				int rankId=pathId%MAX_NUMBER_OF_MPI_PROCESSES;
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE));
				message[0]=pathId;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rankId,TAG_GET_PATH_LENGTH,getRank());
				m_outbox.push_back(aMessage);
				m_fusionData->m_FUSION_pathLengthRequested=true;
				m_fusionData->m_FUSION_pathLengthReceived=false;
			}else if(m_fusionData->m_FUSION_pathLengthReceived){
				m_FINISH_pathLengths[pathId]=m_fusionData->m_FUSION_receivedLength;
			}
		}else if(m_FINISH_pathLengths[pathId]!=(int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()){// avoid fusion of same length.
			int nextPosition=progression+1;
			if(nextPosition<m_FINISH_pathLengths[pathId]){
				// get the vertex
				// get its paths,
				// and continue...
				if(!m_FINISH_vertex_requested){
					int rankId=pathId%MAX_NUMBER_OF_MPI_PROCESSES;
					VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*2);
					message[0]=pathId;
					message[1]=nextPosition;
					Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,rankId,TAG_GET_PATH_VERTEX,getRank());
					m_outbox.push_back(aMessage);
					m_FINISH_vertex_requested=true;
					m_FINISH_vertex_received=false;
				}else if(m_FINISH_vertex_received){
					if(!m_Machine_getPaths_DONE){
						getPaths(m_FINISH_received_vertex);
					}else{
						m_FINISH_pathsForPosition.push_back(m_Machine_getPaths_result);
						m_FINISH_newFusions[m_FINISH_newFusions.size()-1].push_back(m_FINISH_received_vertex);
						m_FINISH_vertex_requested=false;
						m_Machine_getPaths_INITIALIZED=false;
						m_Machine_getPaths_DONE=false;
						m_selectedPosition++;
						m_FINISH_fusionOccured=true;
					}
				}
			}else{
				#ifdef SHOW_FUSION
				cout<<"Ray says: extension-"<<m_ed->m_EXTENSION_identifiers[m_SEEDING_i]<<" ("<<m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()<<" vertices) and extension-"<<pathId<<" ("<<m_FINISH_pathLengths[pathId]<<" vertices) make a fusion, result: "<<m_FINISH_newFusions[m_FINISH_newFusions.size()-1].size()<<" vertices."<<endl;
				#endif

				done=true;
			}
		}else{
			done=true;
		}
	}
	if(done){
		// there is nothing we can do.
		m_SEEDING_i++;
		m_FINISH_vertex_requested=false;
		m_ed->m_EXTENSION_currentPosition=0;
		m_fusionData->m_FUSION_pathLengthRequested=false;
		m_Machine_getPaths_INITIALIZED=false;
		m_Machine_getPaths_DONE=false;
		m_checkedValidity=false;
	}

}

void Machine::makeFusions(){
	// fusion.
	// find a path that matches directly.
	// if a path is 100% included in another, but the other is longer, keep the longest.
	// if a path is 100% identical to another one, keep the one with the lowest ID
	// if a path is 100% identical to another one, but is reverse-complement, keep the one with the lowest ID
	
	int END_LENGTH=100;
	// avoid duplication of contigs.
	if(m_SEEDING_i<(int)m_ed->m_EXTENSION_contigs.size()){
		if((int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()<=END_LENGTH){
			END_LENGTH=m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()-1;
		}
	}
	if(m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_FUSION_DONE,getRank());
		m_outbox.push_back(aMessage);
		m_mode=MODE_DO_NOTHING;
		#ifdef SHOW_PROGRESS
		int seedIndex=m_SEEDING_i-1;
		if(m_ed->m_EXTENSION_contigs.size()==0){
			seedIndex++;
		}
		cout<<"Rank "<<getRank()<<": fusion "<<m_ed->m_EXTENSION_contigs.size()<<"/"<<m_ed->m_EXTENSION_contigs.size()<<" (DONE)"<<endl;
		#endif
		#ifdef DEBUG
		//cout<<"Rank "<<getRank()<<" eliminated: "<<m_fusionData->m_FUSION_eliminated.size()<<endl;
		#endif
		return;
	}else if((int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()<=END_LENGTH){
		#ifdef SHOW_PROGRESS
		cout<<"No fusion for me. "<<m_SEEDING_i<<" "<<m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()<<" "<<m_ed->m_EXTENSION_identifiers[m_SEEDING_i]<<endl;
		#endif
		m_fusionData->m_FUSION_direct_fusionDone=false;
		m_fusionData->m_FUSION_first_done=false;
		m_fusionData->m_FUSION_paths_requested=false;
		m_SEEDING_i++;
		return;
	}else if(!m_fusionData->m_FUSION_direct_fusionDone){
		int currentId=m_ed->m_EXTENSION_identifiers[m_SEEDING_i];
		if(!m_fusionData->m_FUSION_first_done){
			if(!m_fusionData->m_FUSION_paths_requested){
				#ifdef SHOW_PROGRESS
				if(m_SEEDING_i%100==0){
					cout<<"Rank "<<getRank()<<": fusion "<<m_SEEDING_i<<"/"<<m_ed->m_EXTENSION_contigs.size()<<endl;
				}
				#endif
				// get the paths going on the first vertex
				#ifdef DEBUG
				assert((int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()>END_LENGTH);
				#endif
				VERTEX_TYPE theVertex=m_ed->m_EXTENSION_contigs[m_SEEDING_i][END_LENGTH];
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=theVertex;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATHS_SIZE,getRank());
				m_outbox.push_back(aMessage);
				m_fusionData->m_FUSION_paths_requested=true;
				m_fusionData->m_FUSION_paths_received=false;
				m_fusionData->m_FUSION_path_id=0;
				m_fusionData->m_FUSION_path_requested=false;
			}else if(m_fusionData->m_FUSION_paths_received){
				if(m_fusionData->m_FUSION_path_id<m_fusionData->m_FUSION_numberOfPaths){
					if(!m_fusionData->m_FUSION_path_requested){
						VERTEX_TYPE theVertex=m_ed->m_EXTENSION_contigs[m_SEEDING_i][END_LENGTH];
						VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
						message[0]=m_fusionData->m_FUSION_path_id;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATH,getRank());
						m_outbox.push_back(aMessage);
						m_fusionData->m_FUSION_path_requested=true;
						m_fusionData->m_FUSION_path_received=false;
					}else if(m_fusionData->m_FUSION_path_received){
						m_fusionData->m_FUSION_path_id++;
						m_fusionData->m_FUSION_receivedPaths.push_back(m_fusionData->m_FUSION_receivedPath);
						m_fusionData->m_FUSION_path_requested=false;
					}
				}else{
					m_fusionData->m_FUSION_first_done=true;
					m_fusionData->m_FUSION_paths_requested=false;
					m_fusionData->m_FUSION_last_done=false;
					m_fusionData->m_FUSION_firstPaths=m_fusionData->m_FUSION_receivedPaths;
					#ifdef DEBUG
					assert(m_fusionData->m_FUSION_numberOfPaths==(int)m_fusionData->m_FUSION_firstPaths.size());
					#endif
				}
			}
		}else if(!m_fusionData->m_FUSION_last_done){
			// get the paths going on the last vertex.

			if(!m_fusionData->m_FUSION_paths_requested){
				// get the paths going on the lastvertex<
				#ifdef DEBUG
				assert((int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()>=END_LENGTH);
				#endif
				VERTEX_TYPE theVertex=m_ed->m_EXTENSION_contigs[m_SEEDING_i][m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()-END_LENGTH];
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=theVertex;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATHS_SIZE,getRank());
				m_outbox.push_back(aMessage);
				m_fusionData->m_FUSION_paths_requested=true;
				m_fusionData->m_FUSION_paths_received=false;
				m_fusionData->m_FUSION_path_id=0;
				m_fusionData->m_FUSION_path_requested=false;
			}else if(m_fusionData->m_FUSION_paths_received){
				if(m_fusionData->m_FUSION_path_id<m_fusionData->m_FUSION_numberOfPaths){
					if(!m_fusionData->m_FUSION_path_requested){
						VERTEX_TYPE theVertex=m_ed->m_EXTENSION_contigs[m_SEEDING_i][m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()-END_LENGTH];
						VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
						message[0]=m_fusionData->m_FUSION_path_id;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATH,getRank());
						m_outbox.push_back(aMessage);
						m_fusionData->m_FUSION_path_requested=true;
						m_fusionData->m_FUSION_path_received=false;
					}else if(m_fusionData->m_FUSION_path_received){
						m_fusionData->m_FUSION_path_id++;
						m_fusionData->m_FUSION_receivedPaths.push_back(m_fusionData->m_FUSION_receivedPath);
						m_fusionData->m_FUSION_path_requested=false;
					}
				}else{
					m_fusionData->m_FUSION_last_done=true;
					m_fusionData->m_FUSION_paths_requested=false;
					m_fusionData->m_FUSION_lastPaths=m_fusionData->m_FUSION_receivedPaths;
					m_fusionData->m_FUSION_matches_done=false;
					m_fusionData->m_FUSION_matches.clear();

					#ifdef DEBUG
					assert(m_fusionData->m_FUSION_numberOfPaths==(int)m_fusionData->m_FUSION_lastPaths.size());
					#endif
				}
			}


		}else if(!m_fusionData->m_FUSION_matches_done){
			m_fusionData->m_FUSION_matches_done=true;
			map<int,int> index;
			map<int,vector<int> > starts;
			map<int,vector<int> > ends;


			// extract those that are on both starting and ending vertices.
			for(int i=0;i<(int)m_fusionData->m_FUSION_firstPaths.size();i++){
				index[m_fusionData->m_FUSION_firstPaths[i].getWave()]++;
				int pathId=m_fusionData->m_FUSION_firstPaths[i].getWave();
				int progression=m_fusionData->m_FUSION_firstPaths[i].getProgression();
				starts[pathId].push_back(progression);
			}

			vector<int> matches;

			for(int i=0;i<(int)m_fusionData->m_FUSION_lastPaths.size();i++){
				index[m_fusionData->m_FUSION_lastPaths[i].getWave()]++;
				
				int pathId=m_fusionData->m_FUSION_lastPaths[i].getWave();
				int progression=m_fusionData->m_FUSION_lastPaths[i].getProgression();
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
							int expectedLength=m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()-2*END_LENGTH+1;
							//cout<<observedLength<<" versus "<<expectedLength<<endl;
							if(observedLength==expectedLength){
								m_fusionData->m_FUSION_matches.push_back(otherPathId);
								found=true;
								break;
							}
						}
						if(found)
							break;
					}
				}
			}
			if(m_fusionData->m_FUSION_matches.size()==0){ // no match, go next.
				m_fusionData->m_FUSION_direct_fusionDone=true;
				m_fusionData->m_FUSION_reverse_fusionDone=false;
				m_fusionData->m_FUSION_first_done=false;
				m_fusionData->m_FUSION_paths_requested=false;
			}
			m_fusionData->m_FUSION_matches_length_done=false;
			m_fusionData->m_FUSION_match_index=0;
			m_fusionData->m_FUSION_pathLengthRequested=false;
		}else if(!m_fusionData->m_FUSION_matches_length_done){
			int currentId=m_ed->m_EXTENSION_identifiers[m_SEEDING_i];
			if(m_fusionData->m_FUSION_match_index==(int)m_fusionData->m_FUSION_matches.size()){// tested all matches, and nothing was found.
				m_fusionData->m_FUSION_matches_length_done=true;
			}else if(!m_fusionData->m_FUSION_pathLengthRequested){
				int uniquePathId=m_fusionData->m_FUSION_matches[m_fusionData->m_FUSION_match_index];
				int rankId=uniquePathId%MAX_NUMBER_OF_MPI_PROCESSES;
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE));
				message[0]=uniquePathId;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rankId,TAG_GET_PATH_LENGTH,getRank());
				m_outbox.push_back(aMessage);
				m_fusionData->m_FUSION_pathLengthRequested=true;
				m_fusionData->m_FUSION_pathLengthReceived=false;
			}else if(m_fusionData->m_FUSION_pathLengthReceived){
				if(m_fusionData->m_FUSION_receivedLength==0){
				}else if(m_fusionData->m_FUSION_matches[m_fusionData->m_FUSION_match_index]<currentId and m_fusionData->m_FUSION_receivedLength == (int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()){
					m_fusionData->m_FUSION_eliminated.insert(currentId);
					m_fusionData->m_FUSION_direct_fusionDone=false;
					m_fusionData->m_FUSION_first_done=false;
					m_fusionData->m_FUSION_paths_requested=false;
					m_SEEDING_i++;
				}else if(m_fusionData->m_FUSION_receivedLength>(int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size() ){
					m_fusionData->m_FUSION_eliminated.insert(currentId);
					m_fusionData->m_FUSION_direct_fusionDone=false;
					m_fusionData->m_FUSION_first_done=false;
					m_fusionData->m_FUSION_paths_requested=false;
					m_SEEDING_i++;
				}
				m_fusionData->m_FUSION_match_index++;
				m_fusionData->m_FUSION_pathLengthRequested=false;
			}
		}else if(m_fusionData->m_FUSION_matches_length_done){ // no candidate found for fusion.
			m_fusionData->m_FUSION_direct_fusionDone=true;
			m_fusionData->m_FUSION_reverse_fusionDone=false;
			m_fusionData->m_FUSION_first_done=false;
			m_fusionData->m_FUSION_paths_requested=false;
		}
	}else if(!m_fusionData->m_FUSION_reverse_fusionDone){
		int currentId=m_ed->m_EXTENSION_identifiers[m_SEEDING_i];
		if(!m_fusionData->m_FUSION_first_done){
			if(!m_fusionData->m_FUSION_paths_requested){
				// get the paths going on the first vertex
				VERTEX_TYPE theVertex=complementVertex(m_ed->m_EXTENSION_contigs[m_SEEDING_i][m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()-END_LENGTH],m_wordSize,m_colorSpaceMode);
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=theVertex;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATHS_SIZE,getRank());
				m_outbox.push_back(aMessage);
				m_fusionData->m_FUSION_paths_requested=true;
				m_fusionData->m_FUSION_paths_received=false;
				m_fusionData->m_FUSION_path_id=0;
				m_fusionData->m_FUSION_path_requested=false;
			}else if(m_fusionData->m_FUSION_paths_received){
				if(m_fusionData->m_FUSION_path_id<m_fusionData->m_FUSION_numberOfPaths){
					if(!m_fusionData->m_FUSION_path_requested){
						VERTEX_TYPE theVertex=complementVertex(m_ed->m_EXTENSION_contigs[m_SEEDING_i][m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()-END_LENGTH],m_wordSize,m_colorSpaceMode);
						VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
						message[0]=m_fusionData->m_FUSION_path_id;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATH,getRank());
						m_outbox.push_back(aMessage);
						m_fusionData->m_FUSION_path_requested=true;
						m_fusionData->m_FUSION_path_received=false;
					}else if(m_fusionData->m_FUSION_path_received){
						m_fusionData->m_FUSION_path_id++;
						m_fusionData->m_FUSION_receivedPaths.push_back(m_fusionData->m_FUSION_receivedPath);
						m_fusionData->m_FUSION_path_requested=false;
					}
				}else{
					m_fusionData->m_FUSION_first_done=true;
					m_fusionData->m_FUSION_paths_requested=false;
					m_fusionData->m_FUSION_last_done=false;
					m_fusionData->m_FUSION_firstPaths=m_fusionData->m_FUSION_receivedPaths;
					#ifdef DEBUG
					assert(m_fusionData->m_FUSION_numberOfPaths==(int)m_fusionData->m_FUSION_firstPaths.size());
					#endif
				}
			}
		}else if(!m_fusionData->m_FUSION_last_done){
			// get the paths going on the last vertex.

			if(!m_fusionData->m_FUSION_paths_requested){
				// get the paths going on the first vertex
				VERTEX_TYPE theVertex=complementVertex(m_ed->m_EXTENSION_contigs[m_SEEDING_i][END_LENGTH],m_wordSize,m_colorSpaceMode);
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=theVertex;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATHS_SIZE,getRank());
				m_outbox.push_back(aMessage);
				m_fusionData->m_FUSION_paths_requested=true;
				m_fusionData->m_FUSION_paths_received=false;
				m_fusionData->m_FUSION_path_id=0;
				m_fusionData->m_FUSION_path_requested=false;
			}else if(m_fusionData->m_FUSION_paths_received){
				if(m_fusionData->m_FUSION_path_id<m_fusionData->m_FUSION_numberOfPaths){
					if(!m_fusionData->m_FUSION_path_requested){
						VERTEX_TYPE theVertex=complementVertex(m_ed->m_EXTENSION_contigs[m_SEEDING_i][END_LENGTH],m_wordSize,m_colorSpaceMode);
						VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
						message[0]=m_fusionData->m_FUSION_path_id;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(theVertex),TAG_ASK_VERTEX_PATH,getRank());
						m_outbox.push_back(aMessage);
						m_fusionData->m_FUSION_path_requested=true;
						m_fusionData->m_FUSION_path_received=false;
					}else if(m_fusionData->m_FUSION_path_received){
						m_fusionData->m_FUSION_path_id++;
						m_fusionData->m_FUSION_receivedPaths.push_back(m_fusionData->m_FUSION_receivedPath);
						m_fusionData->m_FUSION_path_requested=false;
					}
				}else{
					m_fusionData->m_FUSION_last_done=true;
					m_fusionData->m_FUSION_paths_requested=false;
					m_fusionData->m_FUSION_lastPaths=m_fusionData->m_FUSION_receivedPaths;
					m_fusionData->m_FUSION_matches_done=false;
					m_fusionData->m_FUSION_matches.clear();

					#ifdef DEBUG
					assert(m_fusionData->m_FUSION_numberOfPaths==(int)m_fusionData->m_FUSION_lastPaths.size());
					#endif
				}
			}



		}else if(!m_fusionData->m_FUSION_matches_done){
			m_fusionData->m_FUSION_matches_done=true;
			map<int,int> index;
			map<int,vector<int> > starts;
			map<int,vector<int> > ends;
			for(int i=0;i<(int)m_fusionData->m_FUSION_firstPaths.size();i++){
				index[m_fusionData->m_FUSION_firstPaths[i].getWave()]++;
				int pathId=m_fusionData->m_FUSION_firstPaths[i].getWave();
				int progression=m_fusionData->m_FUSION_firstPaths[i].getProgression();
				starts[pathId].push_back(progression);
			}
			for(int i=0;i<(int)m_fusionData->m_FUSION_lastPaths.size();i++){
				index[m_fusionData->m_FUSION_lastPaths[i].getWave()]++;
				
				int pathId=m_fusionData->m_FUSION_lastPaths[i].getWave();
				int progression=m_fusionData->m_FUSION_lastPaths[i].getProgression();
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
							int expectedLength=m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()-2*END_LENGTH+1;
							//cout<<observedLength<<" versus "<<expectedLength<<endl;
							if(observedLength==expectedLength){
								m_fusionData->m_FUSION_matches.push_back(otherPathId);
								found=true;
								break;
							}
						}
						if(found)
							break;
					}
				}
			}
			if(m_fusionData->m_FUSION_matches.size()==0){ // no match, go next.
				m_fusionData->m_FUSION_direct_fusionDone=false;
				m_fusionData->m_FUSION_first_done=false;
				m_fusionData->m_FUSION_paths_requested=false;
				m_SEEDING_i++;
			}
			m_fusionData->m_FUSION_matches_length_done=false;
			m_fusionData->m_FUSION_match_index=0;
			m_fusionData->m_FUSION_pathLengthRequested=false;
		}else if(!m_fusionData->m_FUSION_matches_length_done){
			int currentId=m_ed->m_EXTENSION_identifiers[m_SEEDING_i];
			if(m_fusionData->m_FUSION_match_index==(int)m_fusionData->m_FUSION_matches.size()){
				m_fusionData->m_FUSION_matches_length_done=true;
			}else if(!m_fusionData->m_FUSION_pathLengthRequested){
				int uniquePathId=m_fusionData->m_FUSION_matches[m_fusionData->m_FUSION_match_index];
				int rankId=uniquePathId%MAX_NUMBER_OF_MPI_PROCESSES;
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE));
				message[0]=uniquePathId;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rankId,TAG_GET_PATH_LENGTH,getRank());
				m_outbox.push_back(aMessage);
				m_fusionData->m_FUSION_pathLengthRequested=true;
				m_fusionData->m_FUSION_pathLengthReceived=false;
			}else if(m_fusionData->m_FUSION_pathLengthReceived){
				if(m_fusionData->m_FUSION_receivedLength==0){
				}else if(m_fusionData->m_FUSION_matches[m_fusionData->m_FUSION_match_index]<currentId and m_fusionData->m_FUSION_receivedLength == (int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()){
					m_fusionData->m_FUSION_eliminated.insert(currentId);
					m_fusionData->m_FUSION_direct_fusionDone=false;
					m_fusionData->m_FUSION_first_done=false;
					m_fusionData->m_FUSION_paths_requested=false;
					m_SEEDING_i++;
				}else if(m_fusionData->m_FUSION_receivedLength>(int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()){
					m_fusionData->m_FUSION_eliminated.insert(currentId);
					m_fusionData->m_FUSION_direct_fusionDone=false;
					m_fusionData->m_FUSION_first_done=false;
					m_fusionData->m_FUSION_paths_requested=false;
					m_SEEDING_i++;
				}
				m_fusionData->m_FUSION_match_index++;
				m_fusionData->m_FUSION_pathLengthRequested=false;
			}
		}else if(m_fusionData->m_FUSION_matches_length_done){ // no candidate found for fusion.
			m_fusionData->m_FUSION_direct_fusionDone=false;
			m_fusionData->m_FUSION_first_done=false;
			m_fusionData->m_FUSION_paths_requested=false;
			m_SEEDING_i++;
		}
	}
}

void Machine::processMessages(){
	for(int i=0;i<(int)m_inbox.size();i++){
		m_mp.processMessage(&(m_inbox[i]),
			m_ed,
			&m_numberOfRanksDoneDetectingDistances,
			&m_numberOfRanksDoneSendingDistances,
			&m_parameters,
			&m_libraryIterator,
			&m_libraryIndexInitiated,
			&m_subgraph,
			&m_outboxAllocator,
			getRank(),
			&m_ed->m_EXTENSION_receivedReads,
			&m_numberOfMachinesDoneSendingEdges,
			m_fusionData,
			&m_ed->m_EXTENSION_contigs,
			&m_wordSize,
			&m_minimumCoverage,
			&m_seedCoverage,
			&m_peakCoverage,
			&m_myReads,
			&m_ed->m_EXTENSION_currentRankIsDone,
			&m_FINISH_newFusions,
		getSize(),
	&m_inboxAllocator,
	&m_persistentAllocator,
	&m_identifiers,
	&m_mode_sendDistribution,
	&m_alive,
	&m_SEEDING_receivedIngoingEdges,
	&m_SEEDING_receivedKey,
	&m_SEEDING_i,
	&m_colorSpaceMode,
	&m_FINISH_fusionOccured,
	&m_Machine_getPaths_INITIALIZED,
	&m_calibration_numberOfMessagesSent,
	&m_mode,
	&m_allPaths,
	&m_ed->m_EXTENSION_VertexAssembled_received,
	&m_ed->m_EXTENSION_numberOfRanksDone,
	&m_ed->m_EXTENSION_currentPosition,
	&m_last_value,
	&m_ed->m_EXTENSION_identifiers,
	&m_ranksDoneAttachingReads,
	&m_SEEDING_edgesReceived,
	&m_ed->m_EXTENSION_pairedRead,
	&m_ed->m_mode_EXTENSION,
	&m_SEEDING_receivedOutgoingEdges,
	&m_DISTRIBUTE_n,
	&m_SEEDING_nodes,
	&m_ed->m_EXTENSION_hasPairedReadReceived,
	&m_numberOfRanksDoneSeeding,
	&m_SEEDING_vertexKeyAndCoverageReceived,
	&m_SEEDING_receivedVertexCoverage,
	&m_ed->m_EXTENSION_readLength_received,
	&m_calibration_MaxSpeed,
	&m_Machine_getPaths_DONE,
	&m_CLEAR_n,
	&m_FINISH_vertex_received,
	&m_ed->m_EXTENSION_initiated,
	&m_readyToSeed,
	&m_SEEDING_NodeInitiated,
	&m_FINISH_n,
	&m_nextReductionOccured,
	&m_ed->m_EXTENSION_hasPairedReadAnswer,
	&m_directionsAllocator,
	&m_FINISH_pathLengths,
	&m_ed->m_EXTENSION_pairedSequenceReceived,
	&m_ed->m_EXTENSION_receivedLength,
	&m_mode_send_coverage_iterator,
	&m_coverageDistribution,
	&m_FINISH_received_vertex,
	&m_ed->m_EXTENSION_read_vertex_received,
	&m_sequence_ready_machines,
	&m_SEEDING_InedgesReceived,
	&m_ed->m_EXTENSION_vertexIsAssembledResult,
	&m_SEEDING_vertexCoverageReceived,
	&m_ed->m_EXTENSION_receivedReadVertex,
	&m_numberOfMachinesReadyForEdgesDistribution,
	&m_numberOfMachinesReadyToSendDistribution,
	&m_mode_send_outgoing_edges,
	&m_mode_send_edge_sequence_id,
	&m_mode_send_vertices_sequence_id,
	&m_mode_send_vertices,
	&m_numberOfMachinesDoneSendingVertices,
	&m_numberOfMachinesDoneSendingCoverage,
	&m_ed->m_EXTENSION_reads_received,
				&m_outbox,
	&m_sd->m_allIdentifiers,&m_oa,
	&m_numberOfRanksWithCoverageData);

	}
	m_inbox.clear();
	m_inboxAllocator.reset();
}

void Machine::detectDistances(){
	if(m_SEEDING_i==(int)m_SEEDING_seeds.size()){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE,getRank());
		m_outbox.push_back(aMessage);
		m_mode=MODE_DO_NOTHING;
	}else if(m_ed->m_EXTENSION_currentPosition==(int)m_SEEDING_seeds[m_SEEDING_i].size()){
		m_ed->m_EXTENSION_currentPosition=0;
		m_SEEDING_i++;
		m_readsPositions.clear();
		#ifdef DEBUG
		assert(m_readsPositions.size()==0);
		#endif
	}else{
		if(!m_ed->m_EXTENSION_reads_requested){
			m_ed->m_EXTENSION_reads_requested=true;
			m_ed->m_EXTENSION_reads_received=false;
			VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
			VERTEX_TYPE vertex=m_SEEDING_seeds[m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
			message[0]=vertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_REQUEST_READS,getRank());
			m_outbox.push_back(aMessage);
			m_ed->m_EXTENSION_edgeIterator=0;// iterate over reads
			m_ed->m_EXTENSION_hasPairedReadRequested=false;
		}else if(m_ed->m_EXTENSION_reads_received){
			if(m_ed->m_EXTENSION_edgeIterator<(int)m_ed->m_EXTENSION_receivedReads.size()){
				ReadAnnotation annotation=m_ed->m_EXTENSION_receivedReads[m_ed->m_EXTENSION_edgeIterator];
				if(!m_ed->m_EXTENSION_hasPairedReadRequested){
					VERTEX_TYPE*message=(VERTEX_TYPE*)(m_outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
					message[0]=annotation.getReadIndex();
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_HAS_PAIRED_READ,getRank());
					(m_outbox).push_back(aMessage);
					m_ed->m_EXTENSION_hasPairedReadRequested=true;
					m_ed->m_EXTENSION_hasPairedReadReceived=false;
					m_ed->m_EXTENSION_readLength_requested=false;
				}else if(m_ed->m_EXTENSION_hasPairedReadReceived){
					if(m_ed->m_EXTENSION_hasPairedReadAnswer){
						if(!m_ed->m_EXTENSION_readLength_requested){
							m_ed->m_EXTENSION_readLength_requested=true;
							m_ed->m_EXTENSION_readLength_received=false;
							VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
							m_ed->m_EXTENSION_pairedSequenceRequested=false;
							message[0]=annotation.getReadIndex();
							Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_ASK_READ_LENGTH,getRank());
							m_outbox.push_back(aMessage);
						}else if(m_ed->m_EXTENSION_readLength_received){
							if(!m_ed->m_EXTENSION_pairedSequenceRequested){
								m_ed->m_EXTENSION_pairedSequenceReceived=false;
								m_ed->m_EXTENSION_pairedSequenceRequested=true;
								VERTEX_TYPE*message=(VERTEX_TYPE*)(m_outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
								message[0]=annotation.getReadIndex();
								Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_GET_PAIRED_READ,getRank());
								(m_outbox).push_back(aMessage);
							}else if(m_ed->m_EXTENSION_pairedSequenceReceived){
								int expectedDeviation=m_ed->m_EXTENSION_pairedRead.getStandardDeviation();
								if(expectedDeviation==_AUTOMATIC_DETECTION){
									int rank=m_ed->m_EXTENSION_pairedRead.getRank();
									int id=m_ed->m_EXTENSION_pairedRead.getId();
									int uniqueReadIdentifier=id*MAX_NUMBER_OF_MPI_PROCESSES+rank;
									if(m_readsPositions.count(uniqueReadIdentifier)>0){
										int library=m_ed->m_EXTENSION_pairedRead.getAverageFragmentLength();
										char currentStrand=annotation.getStrand();
										char otherStrand='F';
										if(currentStrand==otherStrand)
											otherStrand='R';
											
										if(m_readsPositions[uniqueReadIdentifier].count(otherStrand)>0){
											int p1=m_readsPositions[uniqueReadIdentifier][otherStrand];
											
										
											int p2=m_ed->m_EXTENSION_currentPosition;
											int d=p2-p1+m_ed->m_EXTENSION_receivedLength;
											m_libraryDistances[library][d]++;

											#ifdef DEBUG_AUTO
											cout<<"Distance is "<<d<<" (library="<<library<<")"<<endl;
											#endif
										}
									}else{
										#ifdef DEBUG_AUTO
										cout<<"Pair was not found."<<endl;
										#endif
									}
								}
							
								m_ed->m_EXTENSION_edgeIterator++;
								m_ed->m_EXTENSION_hasPairedReadRequested=false;
							}
						}
					}else{
						m_ed->m_EXTENSION_edgeIterator++;
						m_ed->m_EXTENSION_hasPairedReadRequested=false;
					}
				}
			}else{
				#ifdef DEBUG_AUTO
				cout<<"Adding reads in positions "<<m_ed->m_EXTENSION_currentPosition<<endl;
				#endif
				for(int i=0;i<(int)m_ed->m_EXTENSION_receivedReads.size();i++){
					int uniqueId=m_ed->m_EXTENSION_receivedReads[i].getUniqueId();
					int position=m_ed->m_EXTENSION_currentPosition;
					char strand=m_ed->m_EXTENSION_receivedReads[i].getStrand();
					// read, position, strand
					m_readsPositions[uniqueId][strand]=position;
				}

				m_ed->m_EXTENSION_currentPosition++;
				m_ed->m_EXTENSION_reads_requested=false;
			}
		}
	}
}

void Machine::processData(){
	if(m_aborted){
		return;
	}
	
	#ifndef SHOW_PROGRESS
	if(m_readyToSeed==getSize()){
		cout<<"\r"<<"Computing seeds"<<endl;
	}

	if(isMaster() and m_messageSentForVerticesDistribution and m_numberOfMachinesDoneSendingVertices<getSize()){
		time_t tmp=time(NULL);
		if(tmp>m_lastTime){
			m_lastTime=tmp;
			showProgress(m_lastTime);
		}
	}else if(isMaster() and m_messageSentForEdgesDistribution and m_numberOfMachinesDoneSendingEdges<getSize() and m_numberOfMachinesDoneSendingEdges!=-9){
		time_t tmp=time(NULL);
		if(tmp>m_lastTime){
			m_lastTime=tmp;
			showProgress(m_lastTime);
		}
	}else if(isMaster() and m_fusionData->m_fusionStarted  and m_fusionData->m_FUSION_numberOfRanksDone<getSize()){
		time_t tmp=time(NULL);
		if(tmp>m_lastTime){
			m_lastTime=tmp;
			showProgress(m_lastTime);
		}

	}
	#endif

	if(!m_parameters.isInitiated()&&isMaster()){
		if(m_argc==2){
			ifstream f(m_argv[1]);
			if(!f){
				cout<<"Rank "<<getRank()<<" invalid input file."<<endl;
				m_aborted=true;
				f.close();
				killRanks();
				return;
			}
		}

		cout<<endl;
		cout<<"Ray version: "<<m_parameters.getEngineName()<<" "<<m_parameters.getVersion()<<endl;
		cout<<"NumberOfRanks: "<<getSize()<<endl;
		#ifdef OMPI_MPI_H
		cout<<"MPILibrary: Open-MPI "<<OMPI_MAJOR_VERSION<<"."<<OMPI_MINOR_VERSION<<"."<<OMPI_RELEASE_VERSION<<endl;
		#endif
		#ifdef __linux__
		cout<<"OperatingSystem: Linux (during compilation)"<<endl;
		#endif


		m_parameters.load(m_argc,m_argv);
		if(m_parameters.getError()){
			killRanks();
			return;
		}
		if(m_parameters.useAmos()){
			// empty the file.
			cout<<"Preparing AMOS file "<<m_parameters.getAmosFile()<<endl;
			m_bubbleData->m_amos=fopen(m_parameters.getAmosFile().c_str(),"w");
		}

		for(int i=0;i<getSize();i++){
			VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
			message[0]=m_parameters.getWordSize();
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,i,TAG_SET_WORD_SIZE,getRank());
			m_outbox.push_back(aMessage);
			
			VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));

			message2[0]=m_parameters.getColorSpaceMode();
			Message aMessage2(message2,1,MPI_UNSIGNED_LONG_LONG,i,TAG_SET_COLOR_MODE,getRank());
			m_outbox.push_back(aMessage2);
		}
	}else if(m_welcomeStep==true && m_loadSequenceStep==false&&isMaster()){
		bool res=m_sl.loadSequences(getRank(),getSize(),&m_distribution_reads,&m_distribution_sequence_id,
	&m_LOADER_isLeftFile,&m_outbox,&m_distribution_file_id,
	&m_distributionAllocator,&m_LOADER_isRightFile,&m_LOADER_averageFragmentLength,
	m_disData,&m_LOADER_numberOfSequencesInLeftFile,&m_outboxAllocator,
	&m_distribution_currentSequenceId,&m_LOADER_deviation,&m_loadSequenceStep,
	m_bubbleData,
	&m_lastTime,
	&m_parameters
);
		if(!res){
			killRanks();
			m_mode=MODE_DO_NOTHING;
		}

	}else if(m_loadSequenceStep==true && m_mode_send_vertices==false&&isMaster() and m_sequence_ready_machines==getSize()&&m_messageSentForVerticesDistribution==false){
		#ifdef SHOW_PROGRESS
		computeTime(m_startingTime);
		cout<<endl;
		cout<<"Rank "<<getRank()<<": starting vertices distribution."<<endl;
		#else
		cout<<"\r"<<"Counting vertices"<<endl;
		#endif
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG,i, TAG_START_VERTICES_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
		m_messageSentForVerticesDistribution=true;

	
	}else if(m_startEdgeDistribution){
		#ifndef SHOW_PROGRESS
		cout<<"\r"<<"Connecting vertices"<<endl;
		#endif
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG,i,TAG_START_EDGES_DISTRIBUTION_ASK,getRank());
			m_outbox.push_back(aMessage);
		}
		m_startEdgeDistribution=false;
	}else if(m_numberOfMachinesReadyForEdgesDistribution==getSize() and isMaster()){
		computeTime(m_startingTime);
		cout<<endl;
		cout<<"Rank 0 tells its friends to proceed with the distribution of edges."<<endl;
		m_numberOfMachinesReadyForEdgesDistribution=-1;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG,i,TAG_START_EDGES_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
		m_messageSentForEdgesDistribution=true;
	}else if(m_numberOfMachinesDoneSendingCoverage==getSize()){
		m_numberOfMachinesDoneSendingCoverage=-1;
		string file=m_parameters.getCoverageDistributionFile();
		CoverageDistribution distribution(&m_coverageDistribution,&file);
		m_minimumCoverage=distribution.getMinimumCoverage();
		m_peakCoverage=distribution.getPeakCoverage();
		m_seedCoverage=(m_minimumCoverage+m_peakCoverage)/2;

		m_coverageDistribution.clear();

		#ifdef WRITE_PARAMETERS
		ofstream f(m_parameters.getParametersFile().c_str());
		f<<"Ray Command Line: ";
		vector<string> commands=m_parameters.getCommands();
		for(int i=0;i<(int)commands.size();i++){
			f<<" "<<commands[i];
		}
		f<<endl;
		f<<"Ray Assembly Engine: "<<m_parameters.getEngineName()<<"-"<<m_parameters.getVersion()<<endl;
		f<<"Ray Word Size: "<<m_wordSize<<endl;
		f<<"Ray Minimum Coverage: "<<m_minimumCoverage<<endl;
		f<<"Ray Peak Coverage: "<<m_peakCoverage<<endl;
		f<<"Ray Contigs File: "<<m_parameters.getContigsFile()<<endl;
		if(m_parameters.useAmos()){
			f<<"Ray AMOS File: "<<m_parameters.getAmosFile()<<endl;
		}
		f<<"Ray Parameters File: "<<m_parameters.getParametersFile()<<endl;
		f<<"Ray Coverage Distribution File: "<<m_parameters.getCoverageDistributionFile()<<endl;

		f.close();
		cout<<"Writing "<<m_parameters.getParametersFile()<<""<<endl;
		#endif

		if(m_minimumCoverage > m_peakCoverage or m_peakCoverage==m_maxCoverage){
			killRanks();
			cout<<"Error: no enrichment observed."<<endl;
			return;
		}

		// see these values to everyone.
		VERTEX_TYPE*buffer=(VERTEX_TYPE*)m_outboxAllocator.allocate(3*sizeof(VERTEX_TYPE));
		buffer[0]=m_minimumCoverage;
		buffer[1]=m_seedCoverage;
		buffer[2]=m_peakCoverage;
		
		m_numberOfRanksWithCoverageData=0;
		for(int i=0;i<getSize();i++){
			Message aMessage(buffer,3,MPI_UNSIGNED_LONG_LONG,i,TAG_SEND_COVERAGE_VALUES,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_mode_send_vertices==true){
		m_verticesExtractor.process(&m_mode_send_vertices_sequence_id,
				&m_myReads,
				&m_reverseComplementVertex,
				&m_mode_send_vertices_sequence_id_position,
				getRank(),
				&m_outbox,
				&m_mode_send_vertices,
				m_wordSize,
				m_disData,
				getSize(),
				&m_outboxAllocator,
				m_colorSpaceMode
			);
	}else if(m_numberOfRanksWithCoverageData==getSize()){
		m_numberOfRanksWithCoverageData=-1;
		m_numberOfMachinesReadyForEdgesDistribution=getSize();
	}else if(m_numberOfMachinesDoneSendingEdges==getSize()){
		m_numberOfMachinesDoneSendingEdges=-9;

		computeTime(m_startingTime);
		cout<<endl;


		m_mode_AttachSequences=true;
		m_distribution_file_id=m_distribution_sequence_id=m_distribution_currentSequenceId=0;

	}else if(m_numberOfMachinesDoneSendingVertices==getSize()){
		m_numberOfMachinesDoneSendingVertices=-1;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_numberOfMachinesReadyToSendDistribution==getSize()){

		m_numberOfMachinesReadyToSendDistribution=-1;
		computeTime(m_startingTime);
		cout<<endl;
		cout<<"Rank 0 computes the coverage distribution."<<endl;


		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_ranksDoneAttachingReads==getSize()){
		m_ranksDoneAttachingReads=-1;
		m_readyToSeed=getSize();
	}

	if(m_mode==MODE_ASSEMBLE_WAVES){
		// take each seed, and extend it in both direction using previously obtained information.
		if(m_SEEDING_i==(int)m_SEEDING_seeds.size()){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_ASSEMBLE_WAVES_DONE,getRank());
			m_outbox.push_back(aMessage);
		}else{
		}
	}else if(m_mode==MODE_PERFORM_CALIBRATION){
		int rank=rand()%getSize();
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rank,TAG_CALIBRATION_MESSAGE,getRank());
		m_outbox.push_back(aMessage);
		m_calibration_numberOfMessagesSent++;
	}else if(m_mode==MODE_FINISH_FUSIONS){
		finishFusions();
	}else if(m_mode==MODE_DISTRIBUTE_FUSIONS){
		if(m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_DISTRIBUTE_FUSIONS_FINISHED,getRank());
			m_outbox.push_back(aMessage);
			m_mode=MODE_DO_NOTHING;
			return;
		}
	
		VERTEX_TYPE vertex=m_ed->m_EXTENSION_contigs[m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(3*sizeof(VERTEX_TYPE));
		message[0]=vertex;
		message[1]=m_ed->m_EXTENSION_identifiers[m_SEEDING_i];
		message[2]=m_ed->m_EXTENSION_currentPosition;
		Message aMessage(message,3,MPI_UNSIGNED_LONG_LONG,vertexRank(vertex),TAG_SAVE_WAVE_PROGRESSION,getRank());
		m_outbox.push_back(aMessage);

		m_ed->m_EXTENSION_currentPosition++;

		// the next one
		if(m_ed->m_EXTENSION_currentPosition==(int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()){
			m_SEEDING_i++;
			m_ed->m_EXTENSION_currentPosition=0;
		}
	}

	if(m_mode_sendDistribution){
		if(m_distributionOfCoverage.size()==0){
			for(int i=0;i<m_subgraph.getNumberOfTrees();i++){
				SplayTreeIterator<VERTEX_TYPE,Vertex> iterator(m_subgraph.getTree(i));
				while(iterator.hasNext()){
					int coverage=iterator.next()->getValue()->getCoverage();
					m_distributionOfCoverage[coverage]++;
				}
			}
		}

		int length=2;
		VERTEX_TYPE*data=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*length);
		int j=0;
		for(map<int,VERTEX_TYPE>::iterator i=m_distributionOfCoverage.begin();i!=m_distributionOfCoverage.end();i++){
			int coverage=i->first;
			VERTEX_TYPE count=i->second;
			if(m_mode_send_coverage_iterator==j){
				data[0]=coverage;
				data[1]=count;
				break;
			}
			j++;
		}
		Message aMessage(data,length, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, TAG_COVERAGE_DATA,getRank());
		m_outbox.push_back(aMessage);

		m_mode_send_coverage_iterator++;
		if(m_mode_send_coverage_iterator>=(int)m_distributionOfCoverage.size()){
			m_mode_sendDistribution=false;
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG,MASTER_RANK, TAG_COVERAGE_END,getRank());
			m_outbox.push_back(aMessage);
			m_distributionOfCoverage.clear();
		}

	}else if(m_mode_send_outgoing_edges){ 
		#ifdef SHOW_PROGRESS
		if(m_mode_send_edge_sequence_id%100000==0 and m_mode_send_edge_sequence_id_position==0){
			string strand="";
			if(m_reverseComplementEdge)
				strand="(reverse complement)";
			cout<<"Rank "<<getRank()<<" is extracting outgoing edges "<<strand<<" "<<m_mode_send_edge_sequence_id+1<<"/"<<m_myReads.size()<<endl;
		}
		#endif

		if(m_mode_send_edge_sequence_id>(int)m_myReads.size()-1){
			if(m_reverseComplementEdge==false){
				m_mode_send_edge_sequence_id_position=0;
				m_reverseComplementEdge=true;
				flushOutgoingEdges(1);
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<getRank()<<" is extracting outgoing edges "<<m_myReads.size()<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
				#endif
				m_mode_send_edge_sequence_id=0;
			}else{
				flushOutgoingEdges(1);
				m_mode_send_outgoing_edges=false;
				m_mode_send_ingoing_edges=true;
				m_mode_send_edge_sequence_id_position=0;
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<getRank()<<" is extracting outgoing edges (reverse complement) "<<m_myReads.size()<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
				#endif
				m_mode_send_edge_sequence_id=0;
				m_reverseComplementEdge=false;
			}
		}else{
			char*readSequence=m_myReads[m_mode_send_edge_sequence_id]->getSeq();
			int len=strlen(readSequence);

			char memory[100];
			int lll=len-m_wordSize-1;
			if(m_mode_send_edge_sequence_id_position>lll){
				m_mode_send_edge_sequence_id++;
				m_mode_send_edge_sequence_id_position=0;
				return;
			}

			int p=m_mode_send_edge_sequence_id_position;
			memcpy(memory,readSequence+p,m_wordSize+1);
			memory[m_wordSize+1]='\0';
			if(isValidDNA(memory)){
				char prefix[100];
				char suffix[100];
				strcpy(prefix,memory);
				prefix[m_wordSize]='\0';
				strcpy(suffix,memory+1);
				VERTEX_TYPE a_1=wordId(prefix);
				VERTEX_TYPE a_2=wordId(suffix);
				if(m_reverseComplementEdge){
					VERTEX_TYPE b_1=complementVertex(a_2,m_wordSize,m_colorSpaceMode);
					VERTEX_TYPE b_2=complementVertex(a_1,m_wordSize,m_colorSpaceMode);
					int rankB=vertexRank(b_1);
					m_disData->m_messagesStockOut.addAt(rankB,b_1);
					m_disData->m_messagesStockOut.addAt(rankB,b_2);
				}else{
					int rankA=vertexRank(a_1);
					m_disData->m_messagesStockOut.addAt(rankA,a_1);
					m_disData->m_messagesStockOut.addAt(rankA,a_2);
				}
				
				flushOutgoingEdges(MAX_UINT64_T_PER_MESSAGE);
			}
			
			m_mode_send_edge_sequence_id_position++;


		}
	}else if(m_mode_send_ingoing_edges){ 

		#ifdef SHOW_PROGRESS
		if(m_mode_send_edge_sequence_id%100000==0 and m_mode_send_edge_sequence_id_position==0){
			string strand="";
			if(m_reverseComplementEdge)
				strand="(reverse complement)";
			cout<<"Rank "<<getRank()<<" is extracting ingoing edges "<<strand<<" "<<m_mode_send_edge_sequence_id+1<<"/"<<m_myReads.size()<<endl;
		}
		#endif

		if(m_mode_send_edge_sequence_id>(int)m_myReads.size()-1){
			if(m_reverseComplementEdge==false){
				m_reverseComplementEdge=true;
				m_mode_send_edge_sequence_id_position=0;
				flushIngoingEdges(1);
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<getRank()<<" is extracting ingoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
				#endif
				m_mode_send_edge_sequence_id=0;
			}else{
				flushIngoingEdges(1);
				Message aMessage(NULL,0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, TAG_EDGES_DISTRIBUTED,getRank());
				m_outbox.push_back(aMessage);
				m_mode_send_ingoing_edges=false;
			
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<getRank()<<" is extracting ingoing edges (reverse complement) "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
				#endif
			}
		}else{


			char*readSequence=m_myReads[m_mode_send_edge_sequence_id]->getSeq();
			int len=strlen(readSequence);
			char memory[100];
			int lll=len-m_wordSize-1;
			
			if(m_mode_send_edge_sequence_id_position>lll){
				m_mode_send_edge_sequence_id++;
				m_mode_send_edge_sequence_id_position=0;
				return;
			}

				
			memcpy(memory,readSequence+m_mode_send_edge_sequence_id_position,m_wordSize+1);
			memory[m_wordSize+1]='\0';
			if(isValidDNA(memory)){
				char prefix[100];
				char suffix[100];
				strcpy(prefix,memory);
				prefix[m_wordSize]='\0';
				strcpy(suffix,memory+1);
				VERTEX_TYPE a_1=wordId(prefix);
				VERTEX_TYPE a_2=wordId(suffix);
				if(m_reverseComplementEdge){
					VERTEX_TYPE b_1=complementVertex(a_2,m_wordSize,m_colorSpaceMode);
					VERTEX_TYPE b_2=complementVertex(a_1,m_wordSize,m_colorSpaceMode);
					int rankB=vertexRank(b_2);
					m_disData->m_messagesStockIn.addAt(rankB,b_1);
					m_disData->m_messagesStockIn.addAt(rankB,b_2);
				}else{
					int rankA=vertexRank(a_2);
					m_disData->m_messagesStockIn.addAt(rankA,a_1);
					m_disData->m_messagesStockIn.addAt(rankA,a_2);
				}

				// flush data
				flushIngoingEdges(MAX_UINT64_T_PER_MESSAGE);
			}

			m_mode_send_edge_sequence_id_position++;


			if(m_mode_send_edge_sequence_id_position>lll){
				m_mode_send_edge_sequence_id++;
				m_mode_send_edge_sequence_id_position=0;
			}
		}
	}else if(m_readyToSeed==getSize()){
		computeTime(m_startingTime);
		cout<<endl;
		cout<<"Rank 0 tells other ranks to calculate their seeds."<<endl;
		m_readyToSeed=-1;
		m_numberOfRanksDoneSeeding=0;
		// tell everyone to seed now.
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_START_SEEDING,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_mode==MODE_START_SEEDING){

		// assign a first vertex
		if(!m_SEEDING_NodeInitiated){
			if(m_SEEDING_i==(int)m_subgraph.size()-1){
				m_mode=MODE_DO_NOTHING;
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<getRank()<<" is seeding the very vertices it holds. "<<m_SEEDING_i+1<<"/"<<m_subgraph.size()<<" (DONE)"<<endl;
				#endif
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_SEEDING_IS_OVER,getRank());
				m_SEEDING_nodes.clear();
				m_outbox.push_back(aMessage);
			}else{
				#ifdef SHOW_PROGRESS
				if(m_SEEDING_i % 100000 ==0){
					cout<<"Rank "<<getRank()<<" is seeding the very vertices it holds. "<<m_SEEDING_i+1<<"/"<<m_subgraph.size()<<endl;
				}
				#endif
				m_SEEDING_currentVertex=m_SEEDING_nodes[m_SEEDING_i];
				m_SEEDING_first=m_SEEDING_currentVertex;
				m_SEEDING_testInitiated=false;
				m_SEEDING_1_1_test_done=false;
				m_SEEDING_i++;
				m_SEEDING_NodeInitiated=true;
				m_SEEDING_firstVertexTestDone=false;
			}
		// check that this node has 1 ingoing edge and 1 outgoing edge.
		}else if(!m_SEEDING_firstVertexTestDone){
			if(!m_SEEDING_1_1_test_done){
				do_1_1_test();
			}else{
				if(!m_SEEDING_1_1_test_result){
					m_SEEDING_NodeInitiated=false;// abort
				}else{
					m_SEEDING_firstVertexParentTestDone=false;
					m_SEEDING_firstVertexTestDone=true;
					m_SEEDING_currentVertex=m_SEEDING_currentParentVertex;
					m_SEEDING_testInitiated=false;
					m_SEEDING_1_1_test_done=false;
				}
			}

		// check that the parent does not have 1 ingoing edge and 1 outgoing edge
		}else if(!m_SEEDING_firstVertexParentTestDone){
			if(!m_SEEDING_1_1_test_done){
				do_1_1_test();
			}else{
				if(m_SEEDING_1_1_test_result){
					m_SEEDING_NodeInitiated=false;//abort
				}else{
					m_SEEDING_firstVertexParentTestDone=true;
					m_SEEDING_vertices.clear();
					m_SEEDING_seed.clear();
					// restore original starter.
					m_SEEDING_currentVertex=m_SEEDING_first;
					m_SEEDING_testInitiated=false;
					m_SEEDING_1_1_test_done=false;
				}
			}

		// check if currentVertex has 1 ingoing edge and 1 outgoing edge, if yes, add it
		}else{
			// attempt to add m_SEEDING_currentVertex
			if(!m_SEEDING_1_1_test_done){
				do_1_1_test();
			}else{
				if(m_SEEDING_vertices.count(m_SEEDING_currentVertex)>0){
					m_SEEDING_1_1_test_result=false;
				}
				if(!m_SEEDING_1_1_test_result){
					m_SEEDING_NodeInitiated=false;
					int nucleotides=m_SEEDING_seed.size()+m_wordSize-1;
					// only consider the long ones.
					if(nucleotides>=m_parameters.getMinimumContigLength()){
						m_SEEDING_seeds.push_back(m_SEEDING_seed);
					}
				}else{
					m_SEEDING_seed.push_back(m_SEEDING_currentVertex);
					m_SEEDING_vertices.insert(m_SEEDING_currentVertex);
					m_SEEDING_currentVertex=m_SEEDING_currentChildVertex;
					m_SEEDING_testInitiated=false;
					m_SEEDING_1_1_test_done=false;
				}
			}
		}
	}else if(m_numberOfRanksDoneSeeding==getSize()){
		computeTime(m_startingTime);
		cout<<endl;
		cout<<"Rank 0 asks others to approximate library sizes."<<endl;
		m_numberOfRanksDoneSeeding=-1;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_AUTOMATIC_DISTANCE_DETECTION,getRank());
			m_outbox.push_back(aMessage);
		}
		m_numberOfRanksDoneDetectingDistances=0;
	}else if(m_numberOfRanksDoneDetectingDistances==getSize()){
		m_numberOfRanksDoneDetectingDistances=-1;
		m_numberOfRanksDoneSendingDistances=0;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_ASK_LIBRARY_DISTANCES,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_numberOfRanksDoneSendingDistances==getSize()){

		m_numberOfRanksDoneSendingDistances=-1;
		m_parameters.computeAverageDistances();
		m_mode=MODE_DO_NOTHING;
		m_master_mode=MODE_UPDATE_DISTANCES;
		m_fileId=0;
		m_sequence_idInFile=0;
		m_sequence_id=0;


	}else if(m_mode_AttachSequences){
		m_si.attachReads(&m_outbox,&m_distribution_file_id,&m_distribution_sequence_id,
			&m_wordSize,&m_distribution_reads,getSize(),&m_distributionAllocator,
			&m_distribution_currentSequenceId,getRank(),m_disData,&m_mode_AttachSequences,
			&m_parameters,&m_colorSpaceMode,&m_outboxAllocator,&m_lastTime);
	}else if(m_mode==MODE_EXTENSION_ASK and isMaster()){
		
		if(!m_ed->m_EXTENSION_currentRankIsSet){
			m_ed->m_EXTENSION_currentRankIsSet=true;
			m_ed->m_EXTENSION_currentRankIsStarted=false;
			m_ed->m_EXTENSION_rank++;
		}
		if(m_ed->m_EXTENSION_rank==getSize()){
			m_mode=MODE_DO_NOTHING;

		}else if(!m_ed->m_EXTENSION_currentRankIsStarted){
			m_ed->m_EXTENSION_currentRankIsStarted=true;
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,m_ed->m_EXTENSION_rank,TAG_ASK_EXTENSION,getRank());
			m_outbox.push_back(aMessage);
			m_ed->m_EXTENSION_currentRankIsDone=true; // set to false for non-parallel extension.
		}else if(m_ed->m_EXTENSION_currentRankIsDone){
			m_ed->m_EXTENSION_currentRankIsSet=false;
		}

	}else if(m_mode==MODE_SEND_EXTENSION_DATA){
		if(m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){
			m_mode=MODE_DO_NOTHING;
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_DATA_END,getRank());
			m_outbox.push_back(aMessage);
		}else{
			if(m_fusionData->m_FUSION_eliminated.count(m_ed->m_EXTENSION_identifiers[m_SEEDING_i])>0){ // skip merged paths.
				m_SEEDING_i++;
				m_ed->m_EXTENSION_currentPosition=0;
			}else{
				if(m_ed->m_EXTENSION_currentPosition==0){
					VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*1);
					int theId=m_ed->m_EXTENSION_identifiers[m_SEEDING_i];
					message[0]=theId;
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_START,getRank());
					m_outbox.push_back(aMessage);
				}
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*1);
				message[0]=m_ed->m_EXTENSION_contigs[m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
				m_ed->m_EXTENSION_currentPosition++;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_DATA,getRank());
				m_outbox.push_back(aMessage);
				if(m_ed->m_EXTENSION_currentPosition==(int)m_ed->m_EXTENSION_contigs[m_SEEDING_i].size()){
					Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_END,getRank());
					m_outbox.push_back(aMessage);
					m_SEEDING_i++;
					m_ed->m_EXTENSION_currentPosition=0;
				}
			}
		}
	}else if(m_mode==MODE_FUSION){
		makeFusions();
	}else if(m_mode==MODE_AUTOMATIC_DISTANCE_DETECTION){
		detectDistances();
	}else if(m_mode==MODE_SEND_LIBRARY_DISTANCES){
		sendLibraryDistances();
	}else if(m_master_mode==MODE_UPDATE_DISTANCES){
		updateDistances();
	}

	if(m_ed->m_EXTENSION_numberOfRanksDone==getSize()){

		#ifndef SHOW_PROGRESS
		cout<<"\r"<<"Computing fusions"<<endl;
		#endif
		// ask one at once to do the fusion
		// because otherwise it may lead to hanging of the program for unknown reasons
		m_ed->m_EXTENSION_numberOfRanksDone=-1;
		m_master_mode=MODE_DO_NOTHING;
		m_fusionData->m_FUSION_numberOfRanksDone=0;
		for(int i=0;i<(int)getSize();i++){// start fusion.
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_START_FUSION,getRank());
			m_outbox.push_back(aMessage);
		}
		m_fusionData->m_fusionStarted=true;
	}

	if(m_master_mode==MODE_ASSEMBLE_GRAPH){
		// ask ranks to send their extensions.
		if(!m_ed->m_EXTENSION_currentRankIsSet){
			m_ed->m_EXTENSION_currentRankIsSet=true;
			m_ed->m_EXTENSION_currentRankIsStarted=false;
			m_ed->m_EXTENSION_rank++;
		}
		if(m_ed->m_EXTENSION_rank==getSize()){
			m_master_mode=MODE_DO_NOTHING;
			cout<<"Rank "<<getRank()<<" contigs computed."<<endl;
			killRanks();
		}else if(!m_ed->m_EXTENSION_currentRankIsStarted){
			m_ed->m_EXTENSION_currentRankIsStarted=true;
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,m_ed->m_EXTENSION_rank,TAG_ASSEMBLE_WAVES,getRank());
			m_outbox.push_back(aMessage);
			m_ed->m_EXTENSION_currentRankIsDone=false;
		}else if(m_ed->m_EXTENSION_currentRankIsDone){
			m_ed->m_EXTENSION_currentRankIsSet=false;
		}
	}

	if(m_fusionData->m_FUSION_numberOfRanksDone==getSize() and !m_isFinalFusion){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<": fusion is done."<<endl;
		#else
		cout<<"Rank "<<getRank()<<" is finishing fusions."<<endl;
		#endif
		m_fusionData->m_FUSION_numberOfRanksDone=-1;

		m_reductionOccured=true;
		m_cycleStarted=false;
		m_cycleNumber=0;
	}

	if(m_reductionOccured){
		// the finishing is
		//
		//  * a clear cycle
		//  * a distribute cycle
		//  * a finish cycle
		//  * a clear cycle
		//  * a distribute cycle
		//  * a fusion cycle

		if(!m_cycleStarted){
			#ifdef SHOW_PROGRESS
			#endif
			m_nextReductionOccured=false;
			m_cycleStarted=true;
			m_isFinalFusion=false;
			for(int i=0;i<getSize();i++){
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_CLEAR_DIRECTIONS,getRank());
				m_outbox.push_back(aMessage);
			}
	
			m_CLEAR_n=0;
		}else if(m_CLEAR_n==getSize() and !m_isFinalFusion){
			#ifdef SHOW_PROGRESS
			#endif
			m_CLEAR_n=-1;

			for(int i=0;i<getSize();i++){
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_DISTRIBUTE_FUSIONS,getRank());
				m_outbox.push_back(aMessage);
			}
			m_DISTRIBUTE_n=0;
		}else if(m_DISTRIBUTE_n==getSize() and !m_isFinalFusion){
			#ifdef SHOW_PROGRESS
			#endif
			m_DISTRIBUTE_n=-1;
			m_isFinalFusion=true;
			for(int i=0;i<getSize();i++){
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_FINISH_FUSIONS,getRank());
				m_outbox.push_back(aMessage);
			}
			m_FINISH_n=0;
		}else if(m_FINISH_n==getSize() and m_isFinalFusion){
			#ifdef SHOW_PROGRESS
			#endif
			for(int i=0;i<getSize();i++){
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_CLEAR_DIRECTIONS,getRank());
				m_outbox.push_back(aMessage);
			}
			m_FINISH_n=-1;
			m_CLEAR_n=0;
		}else if(m_CLEAR_n==getSize() and m_isFinalFusion){
			m_CLEAR_n=-1;
			#ifdef SHOW_PROGRESS
			#endif

			for(int i=0;i<getSize();i++){
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_DISTRIBUTE_FUSIONS,getRank());
				m_outbox.push_back(aMessage);
			}
			m_DISTRIBUTE_n=0;

		}else if(m_DISTRIBUTE_n==getSize() and m_isFinalFusion){
			#ifdef SHOW_PROGRESS
			computeTime(m_startingTime);
			cout<<endl;
			cout<<"Rank 0 tells others to compute fusions."<<endl;

			#endif
			m_fusionData->m_FUSION_numberOfRanksDone=0;
			m_master_mode=MODE_DO_NOTHING;
			m_DISTRIBUTE_n=-1;
			for(int i=0;i<(int)getSize();i++){// start fusion.
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_START_FUSION,getRank());
				m_outbox.push_back(aMessage);
			}
			
		}else if(m_fusionData->m_FUSION_numberOfRanksDone==getSize() and m_isFinalFusion){
			m_reductionOccured=m_nextReductionOccured;
			m_fusionData->m_FUSION_numberOfRanksDone=-1;
			if(!m_reductionOccured or m_cycleNumber ==5){ // cycling is in development!
				computeTime(m_startingTime);
				cout<<endl;
				cout<<"Rank 0 is "<<"collecting fusions"<<endl;
				m_master_mode=MODE_ASK_EXTENSIONS;

				m_sd->m_computedTopology=false;

				m_sd->m_pathId=0;
				m_sd->m_visitedVertices.clear();
				while(!m_sd->m_verticesToVisit.empty())
					m_sd->m_verticesToVisit.pop();
				while(!m_sd->m_depthsToVisit.empty())
					m_sd->m_depthsToVisit.pop();
				m_sd->m_processedLastVertex=false;
				m_ed->m_EXTENSION_currentRankIsSet=false;
				m_ed->m_EXTENSION_rank=-1;
			}else{
				// we continue now!
				m_cycleStarted=false;
				m_cycleNumber++;
			}
		}
	}

	if(m_master_mode==MODE_ASK_EXTENSIONS){
		#ifndef SHOW_PROGRESS
		time_t tmp=time(NULL);
		if(tmp>m_lastTime){
			m_lastTime=tmp;
			showProgress(m_lastTime);
		}
		#endif

		// ask ranks to send their extensions.
		if(!m_ed->m_EXTENSION_currentRankIsSet){
			m_ed->m_EXTENSION_currentRankIsSet=true;
			m_ed->m_EXTENSION_currentRankIsStarted=false;
			m_ed->m_EXTENSION_rank++;
		}
		if(m_ed->m_EXTENSION_rank==getSize()){
			#ifdef SHOW_SCAFFOLDER
			#endif
			int minimumLength=500;
			if(!m_sd->m_computedTopology){ // in development.
				// for each contig path, take the last vertex, and search for other contig paths 
				// reachable from it.
				if(false and m_sd->m_pathId<(int)m_allPaths.size()){
					int currentPathId=m_identifiers[m_sd->m_pathId];
					if(!m_sd->m_processedLastVertex){
						if((int)m_allPaths[m_sd->m_pathId].size()<minimumLength){
							m_sd->m_pathId++;
							return;
						}
					
						#ifdef SHOW_SCAFFOLDER
						//cout<<"push last vertex."<<endl;
						#endif
						m_sd->m_processedLastVertex=true;
						int theLength=m_allPaths[m_sd->m_pathId].size();
						VERTEX_TYPE lastVertex=m_allPaths[m_sd->m_pathId][theLength-1];
						#ifdef SHOW_SCAFFOLDER
						cout<<"contig-"<<currentPathId<<" Last="<<idToWord(lastVertex,m_wordSize)<<" "<<theLength<<" vertices"<<endl;
		
						#endif
						m_sd->m_verticesToVisit.push(lastVertex);
						m_sd->m_depthsToVisit.push(0);
						m_SEEDING_edgesRequested=false;
						m_sd->m_visitedVertices.clear();
					}else if(!m_sd->m_verticesToVisit.empty()){
						VERTEX_TYPE theVertex=m_sd->m_verticesToVisit.top();
						int theDepth=m_sd->m_depthsToVisit.top();
						if(!m_SEEDING_edgesRequested){
							#ifdef SHOW_SCAFFOLDER
							//cout<<"Asking for arcs. "<<theVertex<<endl;
							#endif
							m_SEEDING_edgesReceived=false;
							m_SEEDING_edgesRequested=true;
							VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
							message[0]=(VERTEX_TYPE)theVertex;
							Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_REQUEST_VERTEX_OUTGOING_EDGES,getRank());
							m_outbox.push_back(aMessage);
							m_Machine_getPaths_DONE=false;
							m_Machine_getPaths_INITIALIZED=false;
							m_Machine_getPaths_result.clear();
							m_sd->m_visitedVertices.insert(theVertex);
						}else if(m_SEEDING_edgesReceived){
							if(!m_Machine_getPaths_DONE){
								getPaths(theVertex);
							}else{
								vector<Direction> nextPaths;
								#ifdef SHOW_SCAFFOLDER
								if(nextPaths.size()>0){
									cout<<"We have "<<nextPaths.size()<<" paths with "<<idToWord(theVertex,m_wordSize)<<endl;	
								}
								#endif
								for(int i=0;i<(int)m_Machine_getPaths_result.size();i++){
									int pathId=m_Machine_getPaths_result[i].getWave();
									if(pathId==currentPathId)
										continue;
									// this one is discarded.
									if(m_sd->m_allIdentifiers.count(pathId)==0){
										continue;
									}
									// not at the front.
									if(m_Machine_getPaths_result[i].getProgression()>0)
										continue;
									int index=m_sd->m_allIdentifiers[pathId];

									// too small to be relevant.
									if((int)m_allPaths[index].size()<minimumLength)
										continue;
									
									#ifdef SHOW_SCAFFOLDER
									#endif
									nextPaths.push_back(m_Machine_getPaths_result[i]);
								}

								m_sd->m_verticesToVisit.pop();
								m_sd->m_depthsToVisit.pop();
								m_SEEDING_edgesRequested=false;

								if(nextPaths.size()>0){// we found a path
									for(int i=0;i<(int)nextPaths.size();i++){
										cout<<"contig-"<<m_identifiers[m_sd->m_pathId]<<" -> "<<"contig-"<<nextPaths[i].getWave()<<" ("<<theDepth<<","<<nextPaths[i].getProgression()<<") via "<<idToWord(theVertex,m_wordSize)<<endl;
										#ifdef DEBUG
										assert(m_sd->m_allIdentifiers.count(nextPaths[i].getWave())>0);
										#endif
									}
								}else{// continue the visit.
									for(int i=0;i<(int)m_SEEDING_receivedOutgoingEdges.size();i++){
										VERTEX_TYPE newVertex=m_SEEDING_receivedOutgoingEdges[i];
										if(m_sd->m_visitedVertices.count(newVertex)>0)
											continue;
										int d=theDepth+1;
										if(d>3000)
											continue;
										m_sd->m_verticesToVisit.push(newVertex);
										m_sd->m_depthsToVisit.push(d);
									}
								}
	
							}
						}
					}else{
						m_sd->m_processedLastVertex=false;
						m_sd->m_pathId++;
						#ifdef SHOW_SCAFFOLDER
						//cout<<"Processing next."<<endl;
						#endif
					}
				}else{
					m_sd->m_computedTopology=true;
				}
				return;
			}

			m_master_mode=MODE_DO_NOTHING;
	
			int totalLength=0;
			
			#ifdef DEBUG
			assert(m_allPaths.size()==m_identifiers.size());
			#endif
			ofstream f(m_parameters.getOutputFile().c_str());
			for(int i=0;i<(int)m_allPaths.size();i++){
				string contig=convertToString(&(m_allPaths[i]),m_wordSize);
				#ifdef DEBUG
				assert(i<(int)m_identifiers.size());
				#endif
				int id=m_identifiers[i];
				#ifdef DEBUG
				int theRank=id%MAX_NUMBER_OF_MPI_PROCESSES;
				assert(theRank<getSize());
				#endif
				f<<">contig-"<<id<<" "<<contig.length()<<" nucleotides"<<endl<<addLineBreaks(contig);
				totalLength+=contig.length();
			}
			f.close();
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getOutputFile()<<endl;
			#else
			cout<<"\r"<<"              "<<endl<<"Writing "<<m_parameters.getOutputFile()<<endl;
			#endif
			cout<<"Rank 0: "<<m_allPaths.size()<<" contigs/"<<totalLength<<" nucleotides"<<endl;
			if(m_parameters.useAmos()){
				m_master_mode=MODE_AMOS;
				m_SEEDING_i=0;
				m_mode_send_vertices_sequence_id_position=0;
				m_ed->m_EXTENSION_reads_requested=false;
				cout<<"\rCompleting "<<m_parameters.getAmosFile()<<endl;
			}else{// we are done.
				killRanks();
			}
			
		}else if(!m_ed->m_EXTENSION_currentRankIsStarted){
			m_ed->m_EXTENSION_currentRankIsStarted=true;
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank()<<" asks "<<m_ed->m_EXTENSION_rank<<" for its fusions."<<endl;
			#endif
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,m_ed->m_EXTENSION_rank,TAG_ASK_EXTENSION_DATA,getRank());
			m_outbox.push_back(aMessage);
			m_ed->m_EXTENSION_currentRankIsDone=false;
		}else if(m_ed->m_EXTENSION_currentRankIsDone){
			m_ed->m_EXTENSION_currentRankIsSet=false;
		}
	}else if(m_master_mode==MODE_AMOS){
		// in development.
		/*
 		* use m_allPaths and m_identifiers
 		*
 		* iterators: m_SEEDING_i: for the current contig
 		*            m_mode_send_vertices_sequence_id_position: for the current position in the current contig.
 		*/
		if(m_SEEDING_i==(int)m_allPaths.size()){// all contigs are processed
			killRanks();
			m_master_mode=MODE_DO_NOTHING;
			fclose(m_bubbleData->m_amos);
		}else if(m_mode_send_vertices_sequence_id_position==(int)m_allPaths[m_SEEDING_i].size()){// iterate over the next one
			m_SEEDING_i++;
			m_mode_send_vertices_sequence_id_position=0;
			m_ed->m_EXTENSION_reads_requested=false;
			
			FILE*fp=m_bubbleData->m_amos;
			fprintf(fp,"}\n");
		}else{
			if(!m_ed->m_EXTENSION_reads_requested){
				if(m_mode_send_vertices_sequence_id_position==0){
					FILE*fp=m_bubbleData->m_amos;
					string seq=convertToString(&(m_allPaths[m_SEEDING_i]),m_wordSize);
					char*qlt=(char*)__Malloc(seq.length()+1);
					strcpy(qlt,seq.c_str());
					for(int i=0;i<(int)strlen(qlt);i++)
						qlt[i]='D';
					fprintf(fp,"{CTG\niid:%i\neid:contig-%i\ncom:\nAssembly engine: %s %s\n.\nseq:\n%s\n.\nqlt:\n%s\n.\n",
						m_SEEDING_i+1,
						m_identifiers[m_SEEDING_i],
						m_parameters.getEngineName().c_str(),
						m_parameters.getVersion().c_str(),
						seq.c_str(),
						qlt
						);
					__Free(qlt);
				}

				m_ed->m_EXTENSION_reads_requested=true;
				m_ed->m_EXTENSION_reads_received=false;
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=m_allPaths[m_SEEDING_i][m_mode_send_vertices_sequence_id_position];
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_REQUEST_READS,getRank());
				m_outbox.push_back(aMessage);

				// iterator on reads
				m_fusionData->m_FUSION_path_id=0;
				m_ed->m_EXTENSION_readLength_requested=false;
			}else if(m_ed->m_EXTENSION_reads_received){
				if(m_fusionData->m_FUSION_path_id<(int)m_ed->m_EXTENSION_receivedReads.size()){
					int readRank=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getRank();
					char strand=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getStrand();
					int idOnRank=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getReadIndex();
					if(!m_ed->m_EXTENSION_readLength_requested){
						m_ed->m_EXTENSION_readLength_requested=true;
						m_ed->m_EXTENSION_readLength_received=false;
						VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
						message[0]=idOnRank;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,readRank,TAG_ASK_READ_LENGTH,getRank());
						m_outbox.push_back(aMessage);
					}else if(m_ed->m_EXTENSION_readLength_received){
						int readLength=m_ed->m_EXTENSION_receivedLength;
						int globalIdentifier=idOnRank*getSize()+readRank;
						FILE*fp=m_bubbleData->m_amos;
						int start=0;
						int theEnd=readLength-1;
						int offset=m_mode_send_vertices_sequence_id_position;
						if(strand=='R'){
							int t=start;
							start=theEnd;
							theEnd=t;
							offset++;
						}
						fprintf(fp,"{TLE\nsrc:%i\noff:%i\nclr:%i,%i\n}\n",globalIdentifier+1,offset,
							start,theEnd);
			
						// increment to get the next read.
						m_fusionData->m_FUSION_path_id++;
						m_ed->m_EXTENSION_readLength_requested=false;
					}
				}else{
					// continue.
					m_mode_send_vertices_sequence_id_position++;
					m_ed->m_EXTENSION_reads_requested=false;
				}
			}

		}
	}

	if(m_ed->m_mode_EXTENSION){

	void extendSeeds(vector<vector<VERTEX_TYPE> >*seeds,ExtensionData*ed,int theRank,vector<Message>*outbox,u64*currentVertex,
	FusionData*fusionData,MyAllocator*outboxAllocator,bool*edgesRequested,int*outgoingEdgeIndex,
int*last_value,bool*vertexCoverageRequested,int wordSize,bool*colorSpaceMode,int size,bool*vertexCoverageReceived,
int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,vector<VERTEX_TYPE>*receivedOutgoingEdges,Chooser*chooser,
ChooserData*cd,BubbleData*bubbleData,DepthFirstSearchData*dfsData,
int minimumCoverage,OpenAssemblerChooser*oa,bool*edgesReceived);

		int maxCoverage=m_maxCoverage;
		m_seedExtender.extendSeeds(&m_SEEDING_seeds,m_ed,getRank(),&m_outbox,&m_SEEDING_currentVertex,
		m_fusionData,&m_outboxAllocator,&m_SEEDING_edgesRequested,&m_SEEDING_outgoingEdgeIndex,
		&m_last_value,&m_SEEDING_vertexCoverageRequested,m_wordSize,&m_colorSpaceMode,getSize(),&m_SEEDING_vertexCoverageReceived,
		&m_SEEDING_receivedVertexCoverage,&m_repeatedLength,&maxCoverage,&m_SEEDING_receivedOutgoingEdges,&m_c,
		m_cd,m_bubbleData,m_dfsData,
	m_minimumCoverage,&m_oa,&m_SEEDING_edgesReceived);
	}
}

/*
 * check if (m_SEEDING_currentRank,m_SEEDING_currentPointer) has
 * 1 ingoing edge and 1 outgoing edge
 *
 * before entering the first call, m_SEEDING_testInitiated and m_SEEDING_1_1_test_done must be false
 *
 * outputs:
 *
 *  m_SEEDING_1_1_test_done
 *  m_SEEDING_currentChildVertex
 *  m_SEEDING_currentChildRank
 *  m_SEEDING_currentChildPointer
 *  m_SEEDING_currentParentRank
 *  m_SEEDING_currentParentPointer
 *
 *
 *  internals:
 *
 *  m_SEEDING_InedgesRequested
 *  m_SEEDING_InedgesReceived
 *  m_SEEDING_Inedge
 *  m_SEEDING_edgesRequested
 *  m_SEEDING_edgesReceived
 */
void Machine::do_1_1_test(){
	if(m_SEEDING_1_1_test_done){
		return;
	}else if(!m_SEEDING_testInitiated){
		m_SEEDING_testInitiated=true;
		m_SEEDING_ingoingEdgesDone=false;
		m_SEEDING_InedgesRequested=false;
	}else if(!m_SEEDING_ingoingEdgesDone){
		if(!m_SEEDING_InedgesRequested){
			VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
			message[0]=(VERTEX_TYPE)m_SEEDING_currentVertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(m_SEEDING_currentVertex),TAG_REQUEST_VERTEX_INGOING_EDGES,getRank());
			m_outbox.push_back(aMessage);
			m_SEEDING_numberOfIngoingEdges=0;
			m_SEEDING_numberOfIngoingEdgesWithSeedCoverage=0;
			m_SEEDING_vertexCoverageRequested=false;
			m_SEEDING_InedgesReceived=false;
			m_SEEDING_InedgesRequested=true;
			m_SEEDING_ingoingEdgeIndex=0;
		}else if(m_SEEDING_InedgesReceived){
			if(m_SEEDING_ingoingEdgeIndex<(int)m_SEEDING_receivedIngoingEdges.size()){
				if(!m_SEEDING_vertexCoverageRequested){
					VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
					message[0]=(VERTEX_TYPE)m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_REQUEST_VERTEX_COVERAGE,getRank());
					m_outbox.push_back(aMessage);
					m_SEEDING_vertexCoverageRequested=true;
					m_SEEDING_vertexCoverageReceived=false;
					m_SEEDING_receivedVertexCoverage=-1;
				}else if(m_SEEDING_vertexCoverageReceived){
					if(m_SEEDING_receivedIngoingEdges.size()==1){//there is only one anyway
						m_SEEDING_currentParentVertex=m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
					}
					if(m_SEEDING_receivedVertexCoverage>=m_seedCoverage){
						m_SEEDING_currentParentVertex=m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
						m_SEEDING_numberOfIngoingEdgesWithSeedCoverage++;
					}
					m_SEEDING_ingoingEdgeIndex++;
					m_SEEDING_numberOfIngoingEdges++;
					m_SEEDING_vertexCoverageRequested=false;
				}
			}else{// done analyzing ingoing edges.
				m_SEEDING_ingoingEdgesDone=true;
				m_SEEDING_outgoingEdgesDone=false;
				m_SEEDING_edgesRequested=false;
			}
		}
	}else if(!m_SEEDING_outgoingEdgesDone){
		if(!m_SEEDING_edgesRequested){
			VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
			message[0]=(VERTEX_TYPE)m_SEEDING_currentVertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(m_SEEDING_currentVertex),TAG_REQUEST_VERTEX_OUTGOING_EDGES,getRank());
			m_outbox.push_back(aMessage);
			m_SEEDING_edgesRequested=true;
			m_SEEDING_numberOfOutgoingEdges=0;
			m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage=0;
			m_SEEDING_vertexCoverageRequested=false;
			m_SEEDING_edgesReceived=false;
			m_SEEDING_outgoingEdgeIndex=0;
		}else if(m_SEEDING_edgesReceived){
			if(m_SEEDING_outgoingEdgeIndex<(int)m_SEEDING_receivedOutgoingEdges.size()){
				// TODO: don't check the coverage if there is only one
				if(!m_SEEDING_vertexCoverageRequested){
					VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
					message[0]=(VERTEX_TYPE)m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_REQUEST_VERTEX_COVERAGE,getRank());
					m_outbox.push_back(aMessage);
					m_SEEDING_vertexCoverageRequested=true;
					m_SEEDING_vertexCoverageReceived=false;
					m_SEEDING_receivedVertexCoverage=-1;
				}else if(m_SEEDING_vertexCoverageReceived){
					if(m_SEEDING_receivedOutgoingEdges.size()==1){//there is only one anyway
						m_SEEDING_currentChildVertex=m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
					}
					if(m_SEEDING_receivedVertexCoverage>=m_seedCoverage){
						m_SEEDING_currentChildVertex=m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
						m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage++;
					}
					m_SEEDING_outgoingEdgeIndex++;
					m_SEEDING_numberOfOutgoingEdges++;
					m_SEEDING_vertexCoverageRequested=false;
				}
			}else{// done analyzing ingoing edges.
				m_SEEDING_outgoingEdgesDone=true;
			}
		}


	}else{
		m_SEEDING_1_1_test_done=true;
		m_SEEDING_1_1_test_result=(m_SEEDING_numberOfIngoingEdgesWithSeedCoverage==1)and
			(m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage==1);
	}
}

void Machine::killRanks(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_GOOD_JOB_SEE_YOU_SOON,getRank());
		m_outbox.push_back(aMessage);
	}
}

bool Machine::isMaster(){
	return getRank()==MASTER_RANK;
}





int Machine::getSize(){
	return m_size;
}



bool Machine::isAlive(){
	return m_alive;
}

void Machine::printStatus(){
	cout<<"********"<<endl;
	cout<<"Rank: "<<getRank()<<endl;
	cout<<"Reads: "<<m_myReads.size()<<endl;
	cout<<"Inbox: "<<m_inbox.size()<<endl;
	cout<<"Outbox: "<<m_outbox.size()<<endl;
}



Machine::~Machine(){
	// do nothing.
	delete m_dfsData;
	delete m_bubbleData;
	m_dfsData=NULL;
	m_bubbleData=NULL;
}


int Machine::vertexRank(VERTEX_TYPE a){
	return hash_VERTEX_TYPE(a)%(getSize());
}

void Machine::updateDistances(){
	if(m_fileId==m_parameters.getNumberOfFiles()){

		computeTime(m_startingTime);
		cout<<endl;
		cout<<"Rank 0 asks others to extend their seeds."<<endl;
		#ifndef SHOW_PROGRESS
		cout<<"\r"<<"Extending seeds"<<endl;
		#endif
		m_mode=MODE_EXTENSION_ASK;
		m_ed->m_EXTENSION_rank=-1;
		m_ed->m_EXTENSION_currentRankIsSet=false;
		m_master_mode=MODE_DO_NOTHING;
	}else{
		if(m_parameters.isRightFile(m_fileId)){
			if(m_parameters.isAutomatic(m_fileId)){
				int library=m_parameters.getLibrary(m_fileId);
				int averageLength=m_parameters.getObservedAverageDistance(library);
				int standardDeviation=m_parameters.getObservedStandardDeviation(library);
				if(m_sequence_idInFile<m_parameters.getNumberOfSequences(m_fileId)){
					int sequenceRank=m_sequence_id%getSize();
					int sequenceIndex=m_sequence_id/getSize();
					u64*message=(u64*)m_outboxAllocator.allocate(3*sizeof(u64));
					message[0]=sequenceIndex;
					message[1]=averageLength;
					message[2]=standardDeviation;
					Message aMessage(message,3,MPI_UNSIGNED_LONG_LONG,sequenceRank,
						TAG_UPDATE_LIBRARY_INFORMATION,getRank());
					m_outbox.push_back(aMessage);

					m_sequence_id++;
					m_sequence_idInFile++;
				}else{
					m_sequence_idInFile=0;
					m_fileId++;
				}
			}else{
				m_sequence_id+=m_parameters.getNumberOfSequences(m_fileId);
				m_fileId++;
				m_sequence_idInFile=0;
			}
		}else{
			m_sequence_id+=m_parameters.getNumberOfSequences(m_fileId);
			m_fileId++;
			m_sequence_idInFile=0;
		}
	}
}
