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


#include<VerticesExtractor.h>
#include<Machine.h>
#include<sstream>
#include<Message.h>
#include<time.h>
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

#define __PAIRED_MULTIPLIER 2
#define __SINGLE_MULTIPLIER 2

using namespace std;

void Machine::showUsage(){
	cout<<"Supported sequences file format: "<<endl;

	cout<<".fasta, .fastq, .sff"<<endl;

	cout<<endl;

	cout<<"Usage:"<<endl;
	cout<<endl;
	cout<<"Ray understands these commands:"<<endl;

    	cout<<" LoadSingleEndReads <sequencesFile> "<<endl;
    	cout<<"  aliases: -s, LoadSingleEndReads, -LoadSingleEndReads, --LoadSingleEndReads"<<endl;
	cout<<endl;
	cout<<" LoadPairedEndReads <leftSequencesFile> <rightSequencesFile> <fragmentLength> <standardDeviation> "<<endl;
	cout<<"  aliases: -p, LoadPairedEndReads, -LoadPairedEndReads, --LoadPairedEndReads"<<endl;
	cout<<endl;
	cout<<" OutputAmosFile "<<endl;
	cout<<"  aliases: -a, OutputAmosFile, -OutputAmosFile, --OutputAmosFile"<<endl;
    	cout<<endl;
	cout<<"Ray receives commands with command-line arguments or with a commands file. "<<endl;


	cout<<endl;
	cout<<"Outputs:"<<endl;
	cout<<" "<<m_parameters.getContigsFile()<<endl;
	cout<<" "<<m_parameters.getAmosFile()<<" (with OutputAmosFile)"<<endl;
	cout<<" "<<m_parameters.getCoverageDistributionFile()<<""<<endl;
	cout<<" "<<m_parameters.getParametersFile()<<endl;
	cout<<endl;
	cout<<"use --help to show this help"<<endl;
	cout<<endl;
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
	m_cd=new ChooserData();
}

void Machine::flushIngoingEdges(int threshold){
	// send messages
	vector<int> toFlush;
	for(map<int,vector<VERTEX_TYPE> >::iterator i=m_disData->m_messagesStockIn.begin();i!=m_disData->m_messagesStockIn.end();i++){
		int destination=i->first;
		int length=i->second.size();
		if(length<threshold)
			continue;
		toFlush.push_back(destination);
		VERTEX_TYPE*data=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*(length));
		for(int j=0;j<(int)i->second.size();j++){
			data[j]=i->second[j];
		}

		Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, TAG_IN_EDGES_DATA,getRank());
		m_outbox.push_back(aMessage);
	}
	for(int i=0;i<(int)toFlush.size();i++){
		m_disData->m_messagesStockIn[toFlush[i]].clear();
	}
}

void Machine::flushOutgoingEdges(int threshold){
	vector<int> toFlush;
	for(map<int,vector<VERTEX_TYPE> >::iterator i=m_disData->m_messagesStockOut.begin();i!=m_disData->m_messagesStockOut.end();i++){
		int destination=i->first;
		int length=i->second.size();
		if(length<threshold)
			continue;
		toFlush.push_back(destination);
		#ifdef SHOW_PROGRESS
		#endif
		VERTEX_TYPE*data=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*(length));
		for(int j=0;j<(int)i->second.size();j++){
			data[j]=i->second[j];
		}

		Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, TAG_OUT_EDGES_DATA,getRank());
		m_outbox.push_back(aMessage);
	}
	for(int i=0;i<(int)toFlush.size();i++){
		m_disData->m_messagesStockOut[toFlush[i]].clear();
	}
}

void Machine::start(){
	m_maxCoverage=0;
	m_maxCoverage--;// underflow.

	#ifdef SHOW_PROGRESS
	cout<<"ProcessIdentifier="<<getpid()<<endl;
	cout<<"sizeof(VERTEX_TYPE) "<<sizeof(VERTEX_TYPE)<<endl;
	cout<<"sizeof(Vertex) "<<sizeof(Vertex)<<endl;
	#endif
	time_t startingTime=time(NULL);
	m_lastTime=time(NULL);
	srand(m_lastTime);
	m_fusionData->m_fusionStarted=false;
	m_EXTENSION_numberOfRanksDone=0;
	m_colorSpaceMode=false;
	m_messageSentForEdgesDistribution=false;
	m_numberOfRanksDoneSeeding=0;
	m_calibrationAskedCalibration=false;
	m_calibrationIsDone=true; // set to false to perform a speed calibration.
	m_master_mode=MODE_DO_NOTHING;
	m_numberOfMachinesReadyForEdgesDistribution=-1;
	m_mode_EXTENSION=false;
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

	MPI_Init(&m_argc,&m_argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&m_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&m_size);



	if(isMaster()){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<" welcomes you to the MPI_COMM_WORLD."<<endl;
		cout<<"Rank "<<getRank()<<": website -> http://denovoassembler.sf.net/"<<endl;
		#ifdef MPICH2_VERSION
		cout<<"Rank "<<getRank()<<": using MPICH2"<<endl;
		#else
			#ifdef OMPI_MPI_H
			cout<<"Rank "<<getRank()<<": using Open-MPI"<<endl;
			#else
			cout<<"Rank "<<getRank()<<": Warning, unknown implementation of MPI"<<endl;
			#endif
		#endif
		#else

		cout<<"Ray Copyright (C) 2010  Sébastien Boisvert, Jacques Corbeil, François Laviolette"<<endl;
    		cout<<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
    		cout<<"This is free software, and you are welcome to redistribute it"<<endl;
    		cout<<"under certain conditions; see \"gpl-3.0.txt\" for details."<<endl;
		cout<<endl;
 		cout<<"see http://denovoassembler.sf.net/"<<endl;
		cout<<endl;

		#endif
	}
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
		time_t endingTime=time(NULL);
		int difference=endingTime-startingTime;
		int minutes=difference/60;
		int seconds=difference%60;
		int hours=minutes/60;
		minutes=minutes%60;
		int days=hours/24;
		hours=hours%24;
		cout<<"\rElapsed time: "<<days<<" d "<<hours<<" h "<<minutes<<" min "<<seconds<<" s"<<endl;
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
		cout<<"Starting "<<m_parameters.getEngineName()<<" "<<m_parameters.getVersion()<<endl;
		cout<<"Ray runs on "<<getSize()<<" MPI processes"<<endl;
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
 * show progress on-screen.
 */
void Machine::showProgress(){
	printf("\r");
	int columns=10;
	int nn=m_lastTime%columns;
	
	for(int i=0;i<nn;i++){
		printf(".");
	}
	for(int i=0;i<columns-nn;i++){
		printf(" ");
	}
	fflush(stdout);

}

void Machine::loadSequences(){
	vector<string> allFiles=m_parameters.getAllFiles();
	if(m_distribution_reads.size()>0 and m_distribution_sequence_id>(int)m_distribution_reads.size()-1){
		// we reached the end of the file.
		m_distribution_file_id++;
		if(m_LOADER_isLeftFile){
			m_LOADER_numberOfSequencesInLeftFile=m_distribution_sequence_id;
		}
		m_distribution_sequence_id=0;
		m_distribution_reads.clear();
	}
	if(m_distribution_file_id>(int)allFiles.size()-1){
		m_loadSequenceStep=true;
		flushPairedStock(1);
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_distributionAllocator.clear();
		m_distribution_reads.clear();
		return;
	}
	if(m_distribution_reads.size()==0){
		Loader loader;
		m_distribution_reads.clear();
		m_distributionAllocator.clear();
		m_distributionAllocator.constructor(DISTRIBUTION_ALLOCATOR_CHUNK_SIZE);
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<" loads "<<allFiles[m_distribution_file_id]<<"."<<endl;
		#else
		cout<<"\r"<<"Loading sequences ("<<allFiles[m_distribution_file_id]<<")"<<endl;
		#endif
		loader.load(allFiles[m_distribution_file_id],&m_distribution_reads,&m_distributionAllocator,&m_distributionAllocator);

		// write Reads in AMOS format.
		if(m_parameters.useAmos()){
			FILE*fp=m_bubbleData->m_amos;
			for(int i=0;i<(int)m_distribution_reads.size();i++){
				int iid=m_distribution_currentSequenceId+i;
				char*seq=m_distribution_reads.at(i)->getSeq();
				char*qlt=(char*)__Malloc(strlen(seq)+1);
				strcpy(qlt,seq);
				// spec: https://sourceforge.net/apps/mediawiki/amos/index.php?title=Message_Types#Sequence_t_:_Universal_t
				for(int j=0;j<(int)strlen(qlt);j++)
					qlt[j]='D';
				fprintf(fp,"{RED\niid:%i\neid:%i\nseq:\n%s\n.\nqlt:\n%s\n.\n}\n",iid+1,iid+1,seq,qlt);
				__Free(qlt);
			}
		}

		if(m_parameters.isLeftFile(m_distribution_file_id)){
			m_LOADER_isLeftFile=true;
		}else if(m_parameters.isRightFile(m_distribution_file_id)){
			m_LOADER_isRightFile=true;
			m_LOADER_averageFragmentLength=m_parameters.getFragmentLength(m_distribution_file_id);
			m_LOADER_deviation=m_parameters.getStandardDeviation(m_distribution_file_id);
		}else{
			m_LOADER_isLeftFile=m_LOADER_isRightFile=false;
		}

		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<" has "<<m_distribution_reads.size()<<" sequences to distribute."<<endl;
		#else
		cout<<"Distributing sequences"<<endl;
		#endif
	}

	#ifndef SHOW_PROGRESS
	time_t tmp=time(NULL);
	if(tmp>m_lastTime){
		m_lastTime=tmp;
		showProgress();
	}
	#endif

	for(int i=0;i<1*getSize();i++){
		if(m_distribution_sequence_id>(int)m_distribution_reads.size()-1){
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank()<<" distributes sequences, "<<m_distribution_reads.size()<<"/"<<m_distribution_reads.size()<<endl;
			#endif
			break;
		}
		int destination=m_distribution_currentSequenceId%getSize();

		if(destination<0 or destination>getSize()-1){
			cout<<destination<<" is bad"<<endl;
		}
		char*sequence=(m_distribution_reads)[m_distribution_sequence_id]->getSeq();
		#ifdef SHOW_PROGRESS
		if(m_distribution_sequence_id%1000000==0){
			cout<<"Rank "<<getRank()<<" distributes sequences, "<<m_distribution_sequence_id<<"/"<<m_distribution_reads.size()<<endl;
		}
		#endif
		Message aMessage(sequence, strlen(sequence), MPI_BYTE, destination, TAG_SEND_SEQUENCE,getRank());
		m_outbox.push_back(aMessage);

		// add paired information here..
		// algorithm follows.
		// check if current file is in a right file.
		// if yes, the leftDistributionCurrentSequenceId=m_distribution_currentSequenceId-NumberOfSequencesInRightFile.
		// the destination of a read i is i%getSize()
		// the readId on destination is i/getSize()
		// so, basically, send these bits to destination:
		//
		// rightSequenceGlobalId:= m_distribution_currentSequenceId
		// rightSequenceRank:= rightSequenceGlobalId%getSize
		// rightSequenceIdOnRank:= rightSequenceGlobalId/getSize
		// leftSequenceGlobalId:= rightSequenceGlobalId-numberOfSequencesInRightFile
		// leftSequenceRank:= leftSequenceGlobalId%getSize
		// leftSequenceIdOnRank:= leftSequenceGlobalId/getSize
		// averageFragmentLength:= ask the pairedFiles in m_parameters.
		if(m_LOADER_isRightFile){
			int rightSequenceGlobalId=m_distribution_currentSequenceId;
			int rightSequenceRank=rightSequenceGlobalId%getSize();
			int rightSequenceIdOnRank=rightSequenceGlobalId/getSize();
			int leftSequenceGlobalId=rightSequenceGlobalId-m_LOADER_numberOfSequencesInLeftFile;
			int leftSequenceRank=leftSequenceGlobalId%getSize();
			int leftSequenceIdOnRank=leftSequenceGlobalId/getSize();
			int averageFragmentLength=m_LOADER_averageFragmentLength;
			int deviation=m_LOADER_deviation;
			m_disData->m_messagesStockPaired[rightSequenceRank].push_back(rightSequenceIdOnRank);
			m_disData->m_messagesStockPaired[rightSequenceRank].push_back(leftSequenceRank);
			m_disData->m_messagesStockPaired[rightSequenceRank].push_back(leftSequenceIdOnRank);
			m_disData->m_messagesStockPaired[rightSequenceRank].push_back(averageFragmentLength);
			m_disData->m_messagesStockPaired[rightSequenceRank].push_back(deviation);
			flushPairedStock(MAX_UINT64_T_PER_MESSAGE);
		}

		m_distribution_currentSequenceId++;
		m_distribution_sequence_id++;
	}
}

void Machine::flushPairedStock(int threshold){
	vector<int> toFlush;
	for(map<int,vector<VERTEX_TYPE> >::iterator i=m_disData->m_messagesStockPaired.begin();
		i!=m_disData->m_messagesStockPaired.end();i++){
		int rightSequenceRank=i->first;
		int count=i->second.size();
		if(count<threshold)
			continue;
		toFlush.push_back(rightSequenceRank);
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(count*sizeof(VERTEX_TYPE));
		for(int j=0;j<count;j++)
			message[j]=i->second[j];
		Message aMessage(message,count,MPI_UNSIGNED_LONG_LONG,rightSequenceRank,TAG_INDEX_PAIRED_SEQUENCE,getRank());
		m_outbox.push_back(aMessage);
	}
	for(int i=0;i<(int)toFlush.size();i++)
		m_disData->m_messagesStockPaired[toFlush[i]].clear();
}

void Machine::attachReads(){
	vector<string> allFiles=m_parameters.getAllFiles();
	if(m_distribution_reads.size()>0 and m_distribution_sequence_id>(int)m_distribution_reads.size()-1){
		m_distribution_file_id++;
		m_distribution_sequence_id=0;
		m_distribution_reads.clear();
	}
	if(m_distribution_file_id>(int)allFiles.size()-1){
		flushAttachedSequences(1);
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_MASTER_IS_DONE_ATTACHING_READS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_distribution_reads.clear();
		m_distributionAllocator.clear();
		m_mode_AttachSequences=false;
		return;
	}
	if(m_distribution_reads.size()==0){
		Loader loader;
		m_distribution_reads.clear();
		m_distributionAllocator.clear();
		m_distributionAllocator.constructor(DISTRIBUTION_ALLOCATOR_CHUNK_SIZE);
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<" loads "<<allFiles[m_distribution_file_id]<<"."<<endl;
		#else
		cout<<"\r"<<"Loading sequences ("<<allFiles[m_distribution_file_id]<<")"<<endl;
		#endif
		loader.load(allFiles[m_distribution_file_id],&m_distribution_reads,&m_distributionAllocator,&m_distributionAllocator);
		
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<" "<<m_distribution_reads.size()<<" sequences to attach"<<endl;
		#else
		cout<<"Indexing sequences"<<endl;
		#endif
	}
	#ifndef SHOW_PROGRESS
	time_t tmp=time(NULL);
	if(tmp>m_lastTime){
		m_lastTime=tmp;
		showProgress();
	}
	#endif

	for(int i=0;i<1;i++){
		if(m_distribution_sequence_id>(int)m_distribution_reads.size()-1){
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank()<<" attaches sequences, "<<m_distribution_reads.size()<<"/"<<m_distribution_reads.size()<<endl;
			#endif
			break;
		}

		int destination=m_distribution_currentSequenceId%getSize();
		int sequenceIdOnDestination=m_distribution_currentSequenceId/getSize();

		if(destination<0 or destination>getSize()-1){
			cout<<destination<<" is bad"<<endl;
		}

		#ifdef SHOW_PROGRESS
		if(m_distribution_sequence_id%1000000==0){
			cout<<"Rank "<<getRank()<<" attaches sequences, "<<m_distribution_sequence_id<<"/"<<m_distribution_reads.size()<<endl;
		}
		#endif

		char*sequence=(m_distribution_reads)[m_distribution_sequence_id]->getSeq();
		if((int)strlen(sequence)<m_wordSize){
			m_distribution_currentSequenceId++;
			m_distribution_sequence_id++;
			return;
		}
		char vertexChar[100];
		memcpy(vertexChar,sequence,m_wordSize);
		vertexChar[m_wordSize]='\0';
		if(isValidDNA(vertexChar)){
			VERTEX_TYPE vertex=wordId(vertexChar);
			int sendTo=vertexRank(vertex);
			m_disData->m_attachedSequence[sendTo].push_back(vertex);
			m_disData->m_attachedSequence[sendTo].push_back(destination);
			m_disData->m_attachedSequence[sendTo].push_back(sequenceIdOnDestination);
			m_disData->m_attachedSequence[sendTo].push_back((VERTEX_TYPE)'F');
		}
		memcpy(vertexChar,sequence+strlen(sequence)-m_wordSize,m_wordSize);
		vertexChar[m_wordSize]='\0';
		if(isValidDNA(vertexChar)){
			VERTEX_TYPE vertex=complementVertex(wordId(vertexChar),m_wordSize,m_colorSpaceMode);
			int sendTo=vertexRank(vertex);
			m_disData->m_attachedSequence[sendTo].push_back(vertex);
			m_disData->m_attachedSequence[sendTo].push_back(destination);
			m_disData->m_attachedSequence[sendTo].push_back(sequenceIdOnDestination);
			m_disData->m_attachedSequence[sendTo].push_back((VERTEX_TYPE)'R');
		}

		flushAttachedSequences(MAX_UINT64_T_PER_MESSAGE);

		m_distribution_currentSequenceId++;
		m_distribution_sequence_id++;
	}

}

void Machine::flushAttachedSequences(int threshold){
	vector<int> toFlush;
	for(map<int,vector<VERTEX_TYPE> >::iterator i=m_disData->m_attachedSequence.begin();
		i!=m_disData->m_attachedSequence.end();i++){
		int sendTo=i->first;
		int count=i->second.size();
		if(count<threshold)
			continue;
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(count*sizeof(VERTEX_TYPE));
		for(int j=0;j<count;j++)
			message[j]=i->second[j];
		Message aMessage(message,count, MPI_UNSIGNED_LONG_LONG,sendTo,TAG_ATTACH_SEQUENCE,getRank());
		m_outbox.push_back(aMessage);
		toFlush.push_back(sendTo);
	}
	for(int i=0;i<(int)toFlush.size();i++)
		m_disData->m_attachedSequence[toFlush[i]].clear();
}



/*
 * finish hyper fusions now!
 */
void Machine::finishFusions(){
	// finishing is broken?
	if(m_SEEDING_i==(int)m_EXTENSION_contigs.size()){
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
		message[0]=m_FINISH_fusionOccured;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_FINISH_FUSIONS_FINISHED,getRank());
		m_outbox.push_back(aMessage);
		m_mode=MODE_DO_NOTHING;
		return;
	}
	int overlapMinimumLength=1000;
	if((int)m_EXTENSION_contigs[m_SEEDING_i].size()<overlapMinimumLength){
		#ifdef SHOW_PROGRESS
		cout<<"No overlap possible m_SEEDING_i="<<m_SEEDING_i<<" size="<<m_EXTENSION_contigs[m_SEEDING_i].size()<<endl;
		#endif
		m_SEEDING_i++;
		m_FINISH_vertex_requested=false;
		m_EXTENSION_currentPosition=0;
		m_fusionData->m_FUSION_pathLengthRequested=false;
		m_Machine_getPaths_INITIALIZED=false;
		m_Machine_getPaths_DONE=false;
		m_checkedValidity=false;
		return;
	}
	// check if the path begins with someone else.
	
	int currentId=m_EXTENSION_identifiers[m_SEEDING_i];
	// don't do it if it is removed.

	// start threading the extension
	// as the algorithm advance on it, it stores the path positions.
	// when it reaches a choice, it will use the available path as basis.
	
	// we have the extension in m_EXTENSION_contigs[m_SEEDING_i]
	// we get the paths with getPaths
	bool done=false;
	if(m_EXTENSION_currentPosition<(int)m_EXTENSION_contigs[m_SEEDING_i].size()){
		if(!m_Machine_getPaths_DONE){
			getPaths(m_EXTENSION_contigs[m_SEEDING_i][m_EXTENSION_currentPosition]);
		}else{
			vector<Direction> a;
			for(int i=0;i<(int)m_Machine_getPaths_result.size();i++){
				if(m_Machine_getPaths_result[i].getWave()!=currentId){
					a.push_back(m_Machine_getPaths_result[i]);
				}
			}
			m_FINISH_pathsForPosition.push_back(a);
			if(m_EXTENSION_currentPosition==0){
				vector<VERTEX_TYPE> a;
				m_FINISH_newFusions.push_back(a);
				m_FINISH_vertex_requested=false;
				m_fusionData->m_FUSION_eliminated.insert(currentId);
				m_fusionData->m_FUSION_pathLengthRequested=false;
				m_checkedValidity=false;
			}
			VERTEX_TYPE vertex=m_EXTENSION_contigs[m_SEEDING_i][m_EXTENSION_currentPosition];
			m_FINISH_newFusions[m_FINISH_newFusions.size()-1].push_back(vertex);
			m_EXTENSION_currentPosition++;
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
		}else if(m_FINISH_pathLengths[pathId]!=(int)m_EXTENSION_contigs[m_SEEDING_i].size()){// avoid fusion of same length.
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
		m_EXTENSION_currentPosition=0;
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
	if(m_SEEDING_i<(int)m_EXTENSION_contigs.size()){
		if((int)m_EXTENSION_contigs[m_SEEDING_i].size()<=END_LENGTH){
			END_LENGTH=m_EXTENSION_contigs[m_SEEDING_i].size()-1;
		}
	}
	if(m_SEEDING_i==(int)m_EXTENSION_contigs.size()){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_FUSION_DONE,getRank());
		m_outbox.push_back(aMessage);
		m_mode=MODE_DO_NOTHING;
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<": fusion "<<m_SEEDING_i-1<<"/"<<m_EXTENSION_contigs.size()<<" (DONE)"<<endl;
		#endif
		#ifdef DEBUG
		//cout<<"Rank "<<getRank()<<" eliminated: "<<m_fusionData->m_FUSION_eliminated.size()<<endl;
		#endif
		return;
	}else if((int)m_EXTENSION_contigs[m_SEEDING_i].size()<=END_LENGTH){
		#ifdef SHOW_PROGRESS
		cout<<"No fusion for me. "<<m_SEEDING_i<<" "<<m_EXTENSION_contigs[m_SEEDING_i].size()<<" "<<m_EXTENSION_identifiers[m_SEEDING_i]<<endl;
		#endif
		m_fusionData->m_FUSION_direct_fusionDone=false;
		m_fusionData->m_FUSION_first_done=false;
		m_fusionData->m_FUSION_paths_requested=false;
		m_SEEDING_i++;
		return;
	}else if(!m_fusionData->m_FUSION_direct_fusionDone){
		int currentId=m_EXTENSION_identifiers[m_SEEDING_i];
		if(!m_fusionData->m_FUSION_first_done){
			if(!m_fusionData->m_FUSION_paths_requested){
				#ifdef SHOW_PROGRESS
				if(m_SEEDING_i%100==0){
					cout<<"Rank "<<getRank()<<": fusion "<<m_SEEDING_i<<"/"<<m_EXTENSION_contigs.size()<<endl;
				}
				#endif
				// get the paths going on the first vertex
				#ifdef DEBUG
				assert((int)m_EXTENSION_contigs[m_SEEDING_i].size()>END_LENGTH);
				#endif
				VERTEX_TYPE theVertex=m_EXTENSION_contigs[m_SEEDING_i][END_LENGTH];
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
						VERTEX_TYPE theVertex=m_EXTENSION_contigs[m_SEEDING_i][END_LENGTH];
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
				assert((int)m_EXTENSION_contigs[m_SEEDING_i].size()>=END_LENGTH);
				#endif
				VERTEX_TYPE theVertex=m_EXTENSION_contigs[m_SEEDING_i][m_EXTENSION_contigs[m_SEEDING_i].size()-END_LENGTH];
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
						VERTEX_TYPE theVertex=m_EXTENSION_contigs[m_SEEDING_i][m_EXTENSION_contigs[m_SEEDING_i].size()-END_LENGTH];
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
							int expectedLength=m_EXTENSION_contigs[m_SEEDING_i].size()-2*END_LENGTH+1;
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
			int currentId=m_EXTENSION_identifiers[m_SEEDING_i];
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
				}else if(m_fusionData->m_FUSION_matches[m_fusionData->m_FUSION_match_index]<currentId and m_fusionData->m_FUSION_receivedLength == (int)m_EXTENSION_contigs[m_SEEDING_i].size()){
					m_fusionData->m_FUSION_eliminated.insert(currentId);
					m_fusionData->m_FUSION_direct_fusionDone=false;
					m_fusionData->m_FUSION_first_done=false;
					m_fusionData->m_FUSION_paths_requested=false;
					m_SEEDING_i++;
				}else if(m_fusionData->m_FUSION_receivedLength>(int)m_EXTENSION_contigs[m_SEEDING_i].size() ){
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
		int currentId=m_EXTENSION_identifiers[m_SEEDING_i];
		if(!m_fusionData->m_FUSION_first_done){
			if(!m_fusionData->m_FUSION_paths_requested){
				// get the paths going on the first vertex
				VERTEX_TYPE theVertex=complementVertex(m_EXTENSION_contigs[m_SEEDING_i][m_EXTENSION_contigs[m_SEEDING_i].size()-END_LENGTH],m_wordSize,m_colorSpaceMode);
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
						VERTEX_TYPE theVertex=complementVertex(m_EXTENSION_contigs[m_SEEDING_i][m_EXTENSION_contigs[m_SEEDING_i].size()-END_LENGTH],m_wordSize,m_colorSpaceMode);
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
				VERTEX_TYPE theVertex=complementVertex(m_EXTENSION_contigs[m_SEEDING_i][END_LENGTH],m_wordSize,m_colorSpaceMode);
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
						VERTEX_TYPE theVertex=complementVertex(m_EXTENSION_contigs[m_SEEDING_i][END_LENGTH],m_wordSize,m_colorSpaceMode);
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
							int expectedLength=m_EXTENSION_contigs[m_SEEDING_i].size()-2*END_LENGTH+1;
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
			int currentId=m_EXTENSION_identifiers[m_SEEDING_i];
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
				}else if(m_fusionData->m_FUSION_matches[m_fusionData->m_FUSION_match_index]<currentId and m_fusionData->m_FUSION_receivedLength == (int)m_EXTENSION_contigs[m_SEEDING_i].size()){
					m_fusionData->m_FUSION_eliminated.insert(currentId);
					m_fusionData->m_FUSION_direct_fusionDone=false;
					m_fusionData->m_FUSION_first_done=false;
					m_fusionData->m_FUSION_paths_requested=false;
					m_SEEDING_i++;
				}else if(m_fusionData->m_FUSION_receivedLength>(int)m_EXTENSION_contigs[m_SEEDING_i].size()){
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
			&m_subgraph,
			&m_outboxAllocator,
			getRank(),
			&m_EXTENSION_receivedReads,
			&m_numberOfMachinesDoneSendingEdges,
			m_fusionData,
			&m_EXTENSION_contigs,
			&m_wordSize,
			&m_minimumCoverage,
			&m_seedCoverage,
			&m_peakCoverage,
			&m_myReads,
			&m_EXTENSION_currentRankIsDone,
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
	&m_EXTENSION_VertexAssembled_received,
	&m_EXTENSION_numberOfRanksDone,
	&m_EXTENSION_currentPosition,
	&m_last_value,
	&m_EXTENSION_identifiers,
	&m_ranksDoneAttachingReads,
	&m_SEEDING_edgesReceived,
	&m_EXTENSION_pairedRead,
	&m_mode_EXTENSION,
	&m_SEEDING_receivedOutgoingEdges,
	&m_DISTRIBUTE_n,
	&m_SEEDING_nodes,
	&m_EXTENSION_hasPairedReadReceived,
	&m_numberOfRanksDoneSeeding,
	&m_SEEDING_vertexKeyAndCoverageReceived,
	&m_SEEDING_receivedVertexCoverage,
	&m_EXTENSION_readLength_received,
	&m_calibration_MaxSpeed,
	&m_Machine_getPaths_DONE,
	&m_CLEAR_n,
	&m_FINISH_vertex_received,
	&m_EXTENSION_initiated,
	&m_readyToSeed,
	&m_SEEDING_NodeInitiated,
	&m_FINISH_n,
	&m_nextReductionOccured,
	&m_EXTENSION_hasPairedReadAnswer,
	&m_directionsAllocator,
	&m_FINISH_pathLengths,
	&m_EXTENSION_pairedSequenceReceived,
	&m_EXTENSION_receivedLength,
	&m_mode_send_coverage_iterator,
	&m_coverageDistribution,
	&m_FINISH_received_vertex,
	&m_EXTENSION_read_vertex_received,
	&m_sequence_ready_machines,
	&m_SEEDING_InedgesReceived,
	&m_EXTENSION_vertexIsAssembledResult,
	&m_SEEDING_vertexCoverageReceived,
	&m_EXTENSION_receivedReadVertex,
	&m_numberOfMachinesReadyForEdgesDistribution,
	&m_numberOfMachinesReadyToSendDistribution,
	&m_mode_send_outgoing_edges,
	&m_mode_send_edge_sequence_id,
	&m_mode_send_vertices_sequence_id,
	&m_mode_send_vertices,
	&m_numberOfMachinesDoneSendingVertices,
	&m_numberOfMachinesDoneSendingCoverage,
	&m_EXTENSION_reads_received,
				&m_outbox);

	}
	m_inbox.clear();
	m_inboxAllocator.reset();
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
			showProgress();
		}
	}else if(isMaster() and m_messageSentForEdgesDistribution and m_numberOfMachinesDoneSendingEdges<getSize() and m_numberOfMachinesDoneSendingEdges!=-9){
		time_t tmp=time(NULL);
		if(tmp>m_lastTime){
			m_lastTime=tmp;
			showProgress();
		}
	}else if(isMaster() and m_fusionData->m_fusionStarted  and m_fusionData->m_FUSION_numberOfRanksDone<getSize()){
		time_t tmp=time(NULL);
		if(tmp>m_lastTime){
			m_lastTime=tmp;
			showProgress();
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
		m_parameters.load(m_argc,m_argv);
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
		loadSequences();
	}else if(m_loadSequenceStep==true && m_mode_send_vertices==false&&isMaster() and m_sequence_ready_machines==getSize()&&m_messageSentForVerticesDistribution==false){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<": starting vertices distribution."<<endl;
		#else
		cout<<"\r"<<"Computing vertices"<<endl;
		#endif
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG,i, TAG_START_VERTICES_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
		m_messageSentForVerticesDistribution=true;
	}else if(m_numberOfMachinesDoneSendingVertices==getSize()){
		m_numberOfMachinesReadyForEdgesDistribution=0;
		m_numberOfMachinesDoneSendingVertices=-1;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_SHOW_VERTICES,getRank());
			m_outbox.push_back(aMessage);
		}


		m_mode_AttachSequences=true;
		m_distribution_file_id=m_distribution_sequence_id=m_distribution_currentSequenceId=0;
		m_startEdgeDistribution=false;
	}else if(m_startEdgeDistribution){
		#ifndef SHOW_PROGRESS
		cout<<"\r"<<"Adding arcs"<<endl;
		#endif
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG,i, TAG_START_EDGES_DISTRIBUTION_ASK,getRank());
			m_outbox.push_back(aMessage);
		}
		m_startEdgeDistribution=false;
	}else if(m_numberOfMachinesReadyForEdgesDistribution==getSize() and isMaster()){
		m_numberOfMachinesReadyForEdgesDistribution=-1;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG,i, TAG_START_EDGES_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
		m_messageSentForEdgesDistribution=true;
	}else if(m_numberOfMachinesDoneSendingCoverage==getSize()){
		m_numberOfMachinesDoneSendingCoverage=-1;
		CoverageDistribution distribution(m_coverageDistribution,m_parameters.getCoverageDistributionFile());
		m_minimumCoverage=distribution.getMinimumCoverage();
		m_peakCoverage=distribution.getPeakCoverage();
		m_seedCoverage=(m_minimumCoverage+m_peakCoverage)/2;

		m_coverageDistribution.clear();

		#ifdef SHOW_PROGRESS
		cout<<"MaxCoverage="<<(int)m_maxCoverage<<endl;
		#endif
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

	}else if(m_numberOfMachinesDoneSendingEdges==getSize()){
		m_numberOfMachinesDoneSendingEdges=-9;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_numberOfMachinesReadyToSendDistribution==getSize()){
		if(m_machineRank<=m_numberOfMachinesDoneSendingCoverage){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, m_machineRank, TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
			m_machineRank++;
		}

		if(m_machineRank==getSize()){
			m_numberOfMachinesReadyToSendDistribution=-1;
		}
	}else if(m_ranksDoneAttachingReads==getSize()){
		m_ranksDoneAttachingReads=-1;
		m_startEdgeDistribution=true;
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
		if(m_SEEDING_i==(int)m_EXTENSION_contigs.size()){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_DISTRIBUTE_FUSIONS_FINISHED,getRank());
			m_outbox.push_back(aMessage);
			m_mode=MODE_DO_NOTHING;
			return;
		}
	
		VERTEX_TYPE vertex=m_EXTENSION_contigs[m_SEEDING_i][m_EXTENSION_currentPosition];
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(3*sizeof(VERTEX_TYPE));
		message[0]=vertex;
		message[1]=m_EXTENSION_identifiers[m_SEEDING_i];
		message[2]=m_EXTENSION_currentPosition;
		Message aMessage(message,3,MPI_UNSIGNED_LONG_LONG,vertexRank(vertex),TAG_SAVE_WAVE_PROGRESSION,getRank());
		m_outbox.push_back(aMessage);

		m_EXTENSION_currentPosition++;

		// the next one
		if(m_EXTENSION_currentPosition==(int)m_EXTENSION_contigs[m_SEEDING_i].size()){
			m_SEEDING_i++;
			m_EXTENSION_currentPosition=0;
		}
	}

	if(m_mode_sendDistribution){
		if(m_distributionOfCoverage.size()==0){
			SplayTreeIterator<VERTEX_TYPE,Vertex> iterator(&m_subgraph);
			while(iterator.hasNext()){
				int coverage=iterator.next()->getValue()->getCoverage();
				m_distributionOfCoverage[coverage]++;
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

	}else if(m_mode_send_outgoing_edges==true){ 
		#ifdef SHOW_PROGRESS
		if(m_mode_send_edge_sequence_id%100000==0 and m_mode_send_edge_sequence_id_position==0){
			string strand="";
			if(m_reverseComplementEdge)
				strand="(reverse complement)";
			cout<<"Rank "<<getRank()<<" is extracting outgoing edges "<<strand<<" "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<endl;
		}
		#endif

		if(m_mode_send_edge_sequence_id>(int)m_myReads.size()-1){
			if(m_reverseComplementEdge==false){
				m_mode_send_edge_sequence_id_position=0;
				m_reverseComplementEdge=true;
				flushOutgoingEdges(1);
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<getRank()<<" is extracting outgoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
				#endif
				m_mode_send_edge_sequence_id=0;
			}else{
				flushOutgoingEdges(1);
				m_mode_send_outgoing_edges=false;
				m_mode_send_ingoing_edges=true;
				m_mode_send_edge_sequence_id_position=0;
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<getRank()<<" is extracting outgoing edges (reverse complement) "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
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

			for(int p=m_mode_send_edge_sequence_id_position;p<=m_mode_send_edge_sequence_id_position;p++){
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
						m_disData->m_messagesStockOut[rankB].push_back(b_1);
						m_disData->m_messagesStockOut[rankB].push_back(b_2);
					}else{
						int rankA=vertexRank(a_1);
						m_disData->m_messagesStockOut[rankA].push_back(a_1);
						m_disData->m_messagesStockOut[rankA].push_back(a_2);
					}
					
				}
			}
			
			flushOutgoingEdges(MAX_UINT64_T_PER_MESSAGE);
			m_mode_send_edge_sequence_id_position++;


		}
	}else if(m_mode_send_ingoing_edges==true){ 

		#ifdef SHOW_PROGRESS
		if(m_mode_send_edge_sequence_id%100000==0 and m_mode_send_edge_sequence_id_position==0){
			string strand="";
			if(m_reverseComplementEdge)
				strand="(reverse complement)";
			cout<<"Rank "<<getRank()<<" is extracting ingoing edges "<<strand<<" "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<endl;
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

			for(int p=m_mode_send_edge_sequence_id_position;p<=m_mode_send_edge_sequence_id_position;p++){
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
						int rankB=vertexRank(b_2);
						m_disData->m_messagesStockIn[rankB].push_back(b_1);
						m_disData->m_messagesStockIn[rankB].push_back(b_2);
					}else{
						int rankA=vertexRank(a_2);
						m_disData->m_messagesStockIn[rankA].push_back(a_1);
						m_disData->m_messagesStockIn[rankA].push_back(a_2);
					}
				}
			}

			m_mode_send_edge_sequence_id_position++;

			// flush data
			flushIngoingEdges(MAX_UINT64_T_PER_MESSAGE);

			if(m_mode_send_edge_sequence_id_position>lll){
				m_mode_send_edge_sequence_id++;
				m_mode_send_edge_sequence_id_position=0;
			}
		}
	}else if(m_readyToSeed==getSize()){
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
				cout<<"Rank "<<getRank()<<" seeding vertices. "<<m_SEEDING_i<<"/"<<m_subgraph.size()<<" (DONE)"<<endl;
				#endif
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_SEEDING_IS_OVER,getRank());
				m_SEEDING_nodes.clear();
				m_outbox.push_back(aMessage);
			}else{
				#ifdef SHOW_PROGRESS
				if(m_SEEDING_i % 100000 ==0){
					cout<<"Rank "<<getRank()<<" seeding vertices. "<<m_SEEDING_i<<"/"<<m_subgraph.size()<<endl;
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
		m_numberOfRanksDoneSeeding=-1;
		#ifndef SHOW_PROGRESS
		cout<<"\r"<<"Extending seeds"<<endl;
		#endif
		m_mode=MODE_EXTENSION_ASK;
		m_EXTENSION_rank=-1;
		m_EXTENSION_currentRankIsSet=false;
	}else if(m_mode_AttachSequences){
		attachReads();
	}else if(m_mode==MODE_EXTENSION_ASK and isMaster()){
		
		if(!m_EXTENSION_currentRankIsSet){
			m_EXTENSION_currentRankIsSet=true;
			m_EXTENSION_currentRankIsStarted=false;
			m_EXTENSION_rank++;
		}
		if(m_EXTENSION_rank==getSize()){
			m_mode=MODE_DO_NOTHING;

		}else if(!m_EXTENSION_currentRankIsStarted){
			m_EXTENSION_currentRankIsStarted=true;
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_rank,TAG_ASK_EXTENSION,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_currentRankIsDone=true; // set to false for non-parallel extension.
		}else if(m_EXTENSION_currentRankIsDone){
			m_EXTENSION_currentRankIsSet=false;
		}

	}else if(m_mode==MODE_SEND_EXTENSION_DATA){
		if(m_SEEDING_i==(int)m_EXTENSION_contigs.size()){
			m_mode=MODE_DO_NOTHING;
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_DATA_END,getRank());
			m_outbox.push_back(aMessage);
		}else{
			if(m_fusionData->m_FUSION_eliminated.count(m_EXTENSION_identifiers[m_SEEDING_i])>0){ // skip merged paths.
				m_SEEDING_i++;
				m_EXTENSION_currentPosition=0;
			}else{
				if(m_EXTENSION_currentPosition==0){
					VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*1);
					int theId=m_EXTENSION_identifiers[m_SEEDING_i];
					message[0]=theId;
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_START,getRank());
					m_outbox.push_back(aMessage);
				}
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(sizeof(VERTEX_TYPE)*1);
				message[0]=m_EXTENSION_contigs[m_SEEDING_i][m_EXTENSION_currentPosition];
				m_EXTENSION_currentPosition++;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_DATA,getRank());
				m_outbox.push_back(aMessage);
				if(m_EXTENSION_currentPosition==(int)m_EXTENSION_contigs[m_SEEDING_i].size()){
					Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_END,getRank());
					m_outbox.push_back(aMessage);
					m_SEEDING_i++;
					m_EXTENSION_currentPosition=0;
				}
			}
		}
	}else if(m_mode==MODE_FUSION){
		makeFusions();
	}

	if(m_EXTENSION_numberOfRanksDone==getSize()){

		#ifndef SHOW_PROGRESS
		cout<<"\r"<<"Computing fusions"<<endl;
		#endif
		// ask one at once to do the fusion
		// because otherwise it may lead to hanging of the program for unknown reasons
		m_EXTENSION_numberOfRanksDone=-1;
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
		if(!m_EXTENSION_currentRankIsSet){
			m_EXTENSION_currentRankIsSet=true;
			m_EXTENSION_currentRankIsStarted=false;
			m_EXTENSION_rank++;
		}
		if(m_EXTENSION_rank==getSize()){
			m_master_mode=MODE_DO_NOTHING;
			cout<<"Rank "<<getRank()<<" contigs computed."<<endl;
			killRanks();
		}else if(!m_EXTENSION_currentRankIsStarted){
			m_EXTENSION_currentRankIsStarted=true;
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_rank,TAG_ASSEMBLE_WAVES,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_currentRankIsDone=false;
		}else if(m_EXTENSION_currentRankIsDone){
			m_EXTENSION_currentRankIsSet=false;
		}
	}

	if(m_fusionData->m_FUSION_numberOfRanksDone==getSize() and !m_isFinalFusion){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<": fusion is done."<<endl;
		#else
		cout<<"\r"<<"Finishing fusions"<<endl;
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
			cout<<"1 TAG_CLEAR_DIRECTIONS"<<endl;
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
			cout<<"2 TAG_DISTRIBUTE_FUSIONS"<<endl;
			#endif
			m_CLEAR_n=-1;

			for(int i=0;i<getSize();i++){
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_DISTRIBUTE_FUSIONS,getRank());
				m_outbox.push_back(aMessage);
			}
			m_DISTRIBUTE_n=0;
		}else if(m_DISTRIBUTE_n==getSize() and !m_isFinalFusion){
			#ifdef SHOW_PROGRESS
			cout<<"3 TAG_FINISH_FUSIONS"<<endl;
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
			cout<<"4 TAG_CLEAR_DIRECTIONS"<<endl;
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
			cout<<"5 TAG_DISTRIBUTE_FUSIONS"<<endl;
			#endif

			for(int i=0;i<getSize();i++){
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_DISTRIBUTE_FUSIONS,getRank());
				m_outbox.push_back(aMessage);
			}
			m_DISTRIBUTE_n=0;

		}else if(m_DISTRIBUTE_n==getSize() and m_isFinalFusion){
			#ifdef SHOW_PROGRESS
			cout<<"6 TAG_START_FUSION"<<endl;
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
				cout<<"\r"<<"Collecting fusions"<<endl;
				m_master_mode=MODE_ASK_EXTENSIONS;
				m_EXTENSION_currentRankIsSet=false;
				m_EXTENSION_rank=-1;
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
			showProgress();
		}
		#endif

		// ask ranks to send their extensions.
		if(!m_EXTENSION_currentRankIsSet){
			m_EXTENSION_currentRankIsSet=true;
			m_EXTENSION_currentRankIsStarted=false;
			m_EXTENSION_rank++;
		}
		if(m_EXTENSION_rank==getSize()){
			m_master_mode=MODE_DO_NOTHING;

			
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
			}
			f.close();
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getOutputFile()<<endl;
			#else
			cout<<"\r"<<"Writing "<<m_parameters.getOutputFile()<<endl;
			#endif
			if(m_parameters.useAmos()){
				m_master_mode=MODE_AMOS;
				m_SEEDING_i=0;
				m_mode_send_vertices_sequence_id_position=0;
				m_EXTENSION_reads_requested=false;
				cout<<"\rCompleting "<<m_parameters.getAmosFile()<<endl;
			}else{// we are done.
				killRanks();
			}
			
		}else if(!m_EXTENSION_currentRankIsStarted){
			m_EXTENSION_currentRankIsStarted=true;
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank()<<" asks "<<m_EXTENSION_rank<<" for its extensions."<<endl;
			#endif
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_rank,TAG_ASK_EXTENSION_DATA,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_currentRankIsDone=false;
		}else if(m_EXTENSION_currentRankIsDone){
			m_EXTENSION_currentRankIsSet=false;
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
			m_EXTENSION_reads_requested=false;
			
			FILE*fp=m_bubbleData->m_amos;
			fprintf(fp,"}\n");
		}else{
			if(!m_EXTENSION_reads_requested){
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

				m_EXTENSION_reads_requested=true;
				m_EXTENSION_reads_received=false;
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=m_allPaths[m_SEEDING_i][m_mode_send_vertices_sequence_id_position];
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_REQUEST_READS,getRank());
				m_outbox.push_back(aMessage);

				// iterator on reads
				m_fusionData->m_FUSION_path_id=0;
				m_EXTENSION_readLength_requested=false;
			}else if(m_EXTENSION_reads_received){
				if(m_fusionData->m_FUSION_path_id<(int)m_EXTENSION_receivedReads.size()){
					int readRank=m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getRank();
					char strand=m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getStrand();
					int idOnRank=m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getReadIndex();
					if(!m_EXTENSION_readLength_requested){
						m_EXTENSION_readLength_requested=true;
						m_EXTENSION_readLength_received=false;
						VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
						message[0]=idOnRank;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,readRank,TAG_ASK_READ_LENGTH,getRank());
						m_outbox.push_back(aMessage);
					}else if(m_EXTENSION_readLength_received){
						int globalIdentifier=idOnRank*getSize()+readRank;
						FILE*fp=m_bubbleData->m_amos;
						int start=0;
						int readLength=m_EXTENSION_receivedLength;
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
						m_EXTENSION_readLength_requested=false;
					}
				}else{
					// continue.
					m_mode_send_vertices_sequence_id_position++;
					m_EXTENSION_reads_requested=false;
				}
			}

		}
	}

	if(m_mode_EXTENSION){
		extendSeeds();
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



void Machine::extendSeeds(){
	if(m_SEEDING_seeds.size()==0){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<": extending seeds "<<m_SEEDING_seeds.size()<<"/"<<m_SEEDING_seeds.size()<<" (DONE)"<<endl;
		#endif
		m_mode_EXTENSION=false;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_IS_DONE,getRank());
		m_outbox.push_back(aMessage);
		return;
	}
	if(!m_EXTENSION_initiated){
		m_EXTENSION_initiated=true;
		m_EXTENSION_currentSeedIndex=0;
		m_EXTENSION_currentPosition=0;
		m_EXTENSION_currentSeed=m_SEEDING_seeds[m_EXTENSION_currentSeedIndex];
		m_SEEDING_currentVertex=m_EXTENSION_currentSeed[m_EXTENSION_currentPosition];
		m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
		m_EXTENSION_directVertexDone=false;
		m_EXTENSION_VertexAssembled_requested=false;
		m_EXTENSION_extension.clear();
		m_EXTENSION_complementedSeed=false;
		m_EXTENSION_reads_startingPositionOnContig.clear();
		m_EXTENSION_readsInRange.clear();
	}else if(m_EXTENSION_currentSeedIndex==(int)m_SEEDING_seeds.size()){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<": extending seeds "<<m_SEEDING_seeds.size()<<"/"<<m_SEEDING_seeds.size()<<" (DONE)"<<endl;
		#endif
		m_mode_EXTENSION=false;
		
		// store the lengths.
		for(int i=0;i<(int)m_EXTENSION_identifiers.size();i++){
			int id=m_EXTENSION_identifiers[i];
			m_fusionData->m_FUSION_identifier_map[id]=i;
		}

		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_IS_DONE,getRank());
		#ifdef SHOW_PROGRESS
		cout<<getRank()<<" TAG_EXTENSION_IS_DONE"<<endl;
		#endif
		m_outbox.push_back(aMessage);
		return;
	}

	

	// algorithms here.
	// if the current vertex is assembled or if its reverse complement is assembled, return
	// else, mark it as assembled, and mark its reverse complement as assembled too.
	// 	enumerate the available choices
	// 	if choices are included in the seed itself
	// 		choose it
	// 	else
	// 		use read paths or pairs of reads to resolve the repeat.
	
	if(!m_EXTENSION_checkedIfCurrentVertexIsAssembled){
		checkIfCurrentVertexIsAssembled();
	}else if(m_EXTENSION_vertexIsAssembledResult and m_EXTENSION_currentPosition==0 and m_EXTENSION_complementedSeed==false){
		m_EXTENSION_currentSeedIndex++;// skip the current one.
		m_EXTENSION_currentPosition=0;
		m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
		m_EXTENSION_directVertexDone=false;
		m_EXTENSION_VertexAssembled_requested=false;
		if(m_EXTENSION_currentSeedIndex<(int)m_SEEDING_seeds.size()){
			m_EXTENSION_currentSeed=m_SEEDING_seeds[m_EXTENSION_currentSeedIndex];
			m_SEEDING_currentVertex=m_EXTENSION_currentSeed[m_EXTENSION_currentPosition];
		}
		// TODO: check if the position !=0
		m_EXTENSION_complementedSeed=false;
		m_EXTENSION_extension.clear();
		m_EXTENSION_reads_startingPositionOnContig.clear();
		m_EXTENSION_readsInRange.clear();
	}else if(!m_EXTENSION_markedCurrentVertexAsAssembled){
		markCurrentVertexAsAssembled();
	}else if(!m_EXTENSION_enumerateChoices){
		enumerateChoices();
	}else if(!m_EXTENSION_choose){
		doChoice();
	}

}

// upon successful completion, m_EXTENSION_coverages and m_enumerateChoices_outgoingEdges are
// populated variables.
void Machine::enumerateChoices(){
	if(!m_SEEDING_edgesRequested){
		m_EXTENSION_coverages.clear();
		m_SEEDING_edgesReceived=false;
		m_SEEDING_edgesRequested=true;
		VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
		message[0]=(VERTEX_TYPE)m_SEEDING_currentVertex;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(m_SEEDING_currentVertex),TAG_REQUEST_VERTEX_OUTGOING_EDGES,getRank());
		m_outbox.push_back(aMessage);
		m_EXTENSION_currentPosition++;
		m_SEEDING_vertexCoverageRequested=false;
		m_SEEDING_outgoingEdgeIndex=0;
	}else if(m_SEEDING_edgesReceived){
		if(m_SEEDING_outgoingEdgeIndex<(int)m_SEEDING_receivedOutgoingEdges.size()){
			// get the coverage of these.
			if(!m_SEEDING_vertexCoverageRequested){
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=(VERTEX_TYPE)m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_REQUEST_VERTEX_COVERAGE,getRank());
				m_outbox.push_back(aMessage);
				m_SEEDING_vertexCoverageRequested=true;
				m_SEEDING_vertexCoverageReceived=false;
				m_SEEDING_receivedVertexCoverage=-1;
			}else if(m_SEEDING_vertexCoverageReceived){
				m_SEEDING_outgoingEdgeIndex++;
				m_SEEDING_vertexCoverageRequested=false;
				m_EXTENSION_coverages.push_back(m_SEEDING_receivedVertexCoverage);
			}
		}else{
			m_EXTENSION_enumerateChoices=true;
			m_EXTENSION_choose=false;
			m_EXTENSION_singleEndResolution=false;
			m_EXTENSION_readIterator=m_EXTENSION_readsInRange.begin();
			m_EXTENSION_readsOutOfRange.clear();
			m_EXTENSION_readLength_done=false;
			m_EXTENSION_readLength_requested=false;
			m_EXTENSION_readPositionsForVertices.clear();
			m_EXTENSION_pairedReadPositionsForVertices.clear();
			
			m_cd->m_CHOOSER_theSumsPaired.clear();
			m_cd->m_CHOOSER_theNumbersPaired.clear();
			m_cd->m_CHOOSER_theMaxsPaired.clear();
			m_cd->m_CHOOSER_theMaxs.clear();
			m_cd->m_CHOOSER_theNumbers.clear();
			m_cd->m_CHOOSER_theSums.clear();

			m_enumerateChoices_outgoingEdges=m_SEEDING_receivedOutgoingEdges;
			

			// nothing to trim...
			if(m_enumerateChoices_outgoingEdges.size()<=1)
				return;

			// avoid unecessary machine instructions
			if(m_EXTENSION_currentPosition<(int)m_EXTENSION_currentSeed.size()){
				return;
			}

			// only keep those with more than 1 coverage.
			vector<int> filteredCoverages;
			vector<VERTEX_TYPE> filteredVertices;
			for(int i=0;i<(int)m_SEEDING_receivedOutgoingEdges.size();i++){
				int coverage=m_EXTENSION_coverages[i];
				VERTEX_TYPE aVertex=m_SEEDING_receivedOutgoingEdges[i];
				#ifdef SHOW_PROGRESS
				#endif
				if(coverage>=_MINIMUM_COVERAGE){
					filteredCoverages.push_back(coverage);
					filteredVertices.push_back(aVertex);
				}
			}
			#ifdef SHOW_PROGRESS
			#endif
			#ifdef SHOW_PROGRESS
			if(filteredCoverages.size()==0)
				cout<<"Now Zero"<<endl;
			#endif
			#ifdef DEBUG
			assert(filteredCoverages.size()==filteredVertices.size());
			assert(m_EXTENSION_coverages.size()==m_SEEDING_receivedOutgoingEdges.size());
			assert(m_enumerateChoices_outgoingEdges.size()==m_SEEDING_receivedOutgoingEdges.size());
			#endif
	
			// toss them in vectors
			#ifdef SHOW_FILTER
			cout<<"FILTER says ";
			for(int i=0;i<(int)m_EXTENSION_coverages.size();i++){
				int coverage=m_EXTENSION_coverages[i];
				VERTEX_TYPE aVertex=m_enumerateChoices_outgoingEdges[i];
				cout<<" ("<<idToWord(aVertex,m_wordSize)<<","<<coverage<<")";
			}
			cout<<" -> ";
			for(int i=0;i<(int)filteredVertices.size();i++){
				int coverage=filteredCoverages[i];
				VERTEX_TYPE aVertex=filteredVertices[i];
				cout<<" ("<<idToWord(aVertex,m_wordSize)<<","<<coverage<<")";
			}
			cout<<" ."<<endl;
			#endif
			#ifdef DEBUG
			assert(filteredVertices.size()<=m_enumerateChoices_outgoingEdges.size());
			assert(filteredCoverages.size()<=m_EXTENSION_coverages.size());
			#endif
			m_EXTENSION_coverages=filteredCoverages;
			m_enumerateChoices_outgoingEdges=filteredVertices;
			#ifdef DEBUG
			assert(m_EXTENSION_coverages.size()==m_enumerateChoices_outgoingEdges.size());
			#endif
		}
	}
}

int Machine::proceedWithCoverages(int a,int b){
	vector<int> counts2;
	vector<int> counts5;
	for(int i=0;i<(int)m_EXTENSION_coverages.size();i++){
		int j2=0;
		int j5=0;
		if(m_EXTENSION_readPositionsForVertices.count(i)>0){
			for(int k=0;k<(int)m_EXTENSION_readPositionsForVertices[i].size();k++){
				int distanceFromOrigin=m_EXTENSION_readPositionsForVertices[i][k];
				if(distanceFromOrigin>=2){
					j2++;
				}
				if(distanceFromOrigin>=5){
					j5++;
				}
			}
		}
		counts2.push_back(j2);
		counts5.push_back(j5);
	}

	for(int i=0;i<(int)m_EXTENSION_coverages.size();i++){
		bool isBetter=true;
		int coverageI=m_EXTENSION_coverages[i];
		int singleReadsI=m_EXTENSION_readPositionsForVertices[i].size();
		if(counts2[i]==0)
			continue;

		// in less than 10% of the coverage is supported by displayed reads, abort it...
		if(singleReadsI*10 < coverageI){
			continue;
		}

		for(int j=0;j<(int)m_EXTENSION_coverages.size();j++){
			if(i==j)
				continue;
			//int coverageJ=m_EXTENSION_coverages[j];
			int singleReadsJ=m_EXTENSION_readPositionsForVertices[j].size();
			if(!(singleReadsJ<=a and singleReadsI>=b)){
				isBetter=false;
				break;
			}

			// too much coverage is not good at all, sir
			if(coverageI==m_maxCoverage){
			}
		}
		if(isBetter){
			#ifdef SHOW_CHOICE
			cout<<"Choice #"<<i+1<<" wins, with "<<m_EXTENSION_readPositionsForVertices[i].size()<<" reads."<<endl;
			cout<<" in ranges: "<<m_EXTENSION_readsInRange.size()<<endl;
			#endif
			m_SEEDING_currentVertex=m_enumerateChoices_outgoingEdges[i];
			m_EXTENSION_choose=true;
			m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
			m_EXTENSION_directVertexDone=false;
			m_EXTENSION_VertexAssembled_requested=false;

			return i;
		}
	}
	return -1;
}

#define _UPDATE_SINGLE_VALUES(d) \
if(m_cd->m_CHOOSER_theMaxs.count(m_EXTENSION_edgeIterator)==0){ \
	m_cd->m_CHOOSER_theSums[m_EXTENSION_edgeIterator]=0; \
	m_cd->m_CHOOSER_theMaxs[m_EXTENSION_edgeIterator]=distance; \
	m_cd->m_CHOOSER_theNumbers[m_EXTENSION_edgeIterator]=0; \
}\
if(distance>m_cd->m_CHOOSER_theMaxs[m_EXTENSION_edgeIterator])\
	m_cd->m_CHOOSER_theMaxs[m_EXTENSION_edgeIterator]=distance;\
m_cd->m_CHOOSER_theNumbers[m_EXTENSION_edgeIterator]++;\
m_cd->m_CHOOSER_theSums[m_EXTENSION_edgeIterator]+=distance;


/**
 *
 *  This function do a choice:
 *   IF the position is inside the given seed, THEN the seed is used as a backbone to do the choice
 *   IF the position IS NOT inside the given seed, THEN
 *      reads in range are mapped on available choices.
 *      then, Ray attempts to choose with paired-end reads
 *      if this fails, Ray attempts to choose with single-end reads
 *      if this fails, Ray attempts to choose with only the number of reads covering arcs
 *      if this fails, Ray attempts to choose by removing tips.
 *      if this fails, Ray attempts to choose by resolving bubbles (NOT IMPLEMENTED YET)
 */
void Machine::doChoice(){
	// use seed information.
	#ifdef SHOW_PROGRESS
	if(m_EXTENSION_currentPosition==1)
		cout<<"Priming with seed length="<<m_EXTENSION_currentSeed.size()<<endl;
	#endif
	
	// use the seed to extend the thing.
	if(m_EXTENSION_currentPosition<(int)m_EXTENSION_currentSeed.size()){
		#ifdef SHOW_EXTEND_WITH_SEED
		cout<<"Extending with seed, p="<<m_EXTENSION_currentPosition<<endl;
		#endif

		// a speedy test follows, using mighty MACROs
		#define _UNROLLED_LOOP(i) if(m_enumerateChoices_outgoingEdges.size()>=(i+1)){ \
			if(m_enumerateChoices_outgoingEdges[i]==m_EXTENSION_currentSeed[m_EXTENSION_currentPosition]){ \
				m_SEEDING_currentVertex=m_enumerateChoices_outgoingEdges[i]; \
				m_EXTENSION_choose=true; \
				m_EXTENSION_checkedIfCurrentVertexIsAssembled=false; \
				m_EXTENSION_directVertexDone=false; \
				m_EXTENSION_VertexAssembled_requested=false; \
				return; \
			}\
		}
		_UNROLLED_LOOP(0);
		_UNROLLED_LOOP(1);
		_UNROLLED_LOOP(2);
		_UNROLLED_LOOP(3);
		#ifdef SHOW_EXTEND_WITH_SEED
		cout<<"What the hell? position="<<m_EXTENSION_currentPosition<<" "<<idToWord(m_EXTENSION_currentSeed[m_EXTENSION_currentPosition],m_wordSize)<<" with choices ";
		for(int j=0;j<(int)m_enumerateChoices_outgoingEdges.size();j++){
			cout<<" "<<idToWord(m_enumerateChoices_outgoingEdges[j],m_wordSize)<<endl;
		}
		cout<<endl;
		#endif

		#ifdef DEBUG
		assert(false);
		#endif

	// else, do a paired-end or single-end lookup if reads are in range.
	}else{

/*
 *
 *                         min                          seed                       peak
 *             min/2       
 *                                      2min
 *   A         ==============
 *   B                      =============================
 *   C                      =============
 */
		// stuff in the reads to appropriate arcs.
		if(!m_EXTENSION_singleEndResolution and m_EXTENSION_readsInRange.size()>0){
			// try to use single-end reads to resolve the repeat.
			// for each read in range, ask them their vertex at position (CurrentPositionOnContig-StartPositionOfReadOnContig)
			// and cumulate the results in
			// m_EXTENSION_readPositions, which is a map<int,vector<int> > if one of the vertices match
			if(m_EXTENSION_readIterator!=m_EXTENSION_readsInRange.end()){
				if(!m_EXTENSION_readLength_done){
					if(!m_EXTENSION_readLength_requested){
						m_EXTENSION_readLength_requested=true;
						m_EXTENSION_readLength_received=false;

						ReadAnnotation annotation=*m_EXTENSION_readIterator;
						VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
						message[0]=annotation.getReadIndex();
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_ASK_READ_LENGTH,getRank());
						m_outbox.push_back(aMessage);
					}else if(m_EXTENSION_readLength_received){
						ReadAnnotation annotation=*m_EXTENSION_readIterator;

						int startPosition=m_EXTENSION_reads_startingPositionOnContig[annotation.getUniqueId()];
						int distance=m_EXTENSION_extension.size()-startPosition;
						if(distance>(m_EXTENSION_receivedLength-m_wordSize)){
							// the read is now out-of-range.
							m_EXTENSION_readsOutOfRange.push_back(annotation);
							m_EXTENSION_readLength_done=false;
							m_EXTENSION_readLength_requested=false;
							m_EXTENSION_readIterator++;
						}else{
							//the read is in-range
							m_EXTENSION_readLength_done=true;
							m_EXTENSION_read_vertex_requested=false;
						}
					}
				}else if(!m_EXTENSION_read_vertex_requested){
					// request the vertex for the read
					m_EXTENSION_read_vertex_requested=true;
					ReadAnnotation annotation=*m_EXTENSION_readIterator;
					int startPosition=m_EXTENSION_reads_startingPositionOnContig[annotation.getUniqueId()];
					if(!(0<=startPosition and startPosition<(int)m_EXTENSION_extension.size())){
						cout<<"FATAL"<<endl;
						cout<<"The read started at "<<startPosition<<endl;
						cout<<"The extension has "<<m_EXTENSION_extension.size()<<" elements."<<endl;
						cout<<"The read has length="<<m_EXTENSION_receivedLength<<endl;
					}
					int distance=m_EXTENSION_extension.size()-startPosition;
					VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(3*sizeof(VERTEX_TYPE));
					message[0]=annotation.getReadIndex();
					message[1]=distance;
					message[2]=annotation.getStrand();
					Message aMessage(message,3,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_ASK_READ_VERTEX_AT_POSITION,getRank());
					m_outbox.push_back(aMessage);
					m_EXTENSION_read_vertex_received=false;
					m_EXTENSION_edgeIterator=0;
					m_EXTENSION_hasPairedReadRequested=false;
				}else if(m_EXTENSION_read_vertex_received){
					// we received the vertex for that read,
					// now check if it matches one of 
					// the many choices we have
					ReadAnnotation annotation=*m_EXTENSION_readIterator;
					int startPosition=m_EXTENSION_reads_startingPositionOnContig[annotation.getUniqueId()];
					int distance=m_EXTENSION_extension.size()-startPosition;

					// process each edge separately.
					if(m_EXTENSION_edgeIterator<(int)m_enumerateChoices_outgoingEdges.size()){
						// got a match!
						if(m_EXTENSION_receivedReadVertex==m_enumerateChoices_outgoingEdges[m_EXTENSION_edgeIterator]){
							ReadAnnotation annotation=*m_EXTENSION_readIterator;
							// check if the current read has a paired read.
							if(!m_EXTENSION_hasPairedReadRequested){
								VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
								message[0]=annotation.getReadIndex();
								Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_HAS_PAIRED_READ,getRank());
								m_outbox.push_back(aMessage);
								m_EXTENSION_hasPairedReadRequested=true;
								m_EXTENSION_hasPairedReadReceived=false;
								m_EXTENSION_pairedSequenceRequested=false;
							}else if(m_EXTENSION_hasPairedReadReceived){
								// vertex matches, but no paired end read found, at last.
								if(!m_EXTENSION_hasPairedReadAnswer){
									m_EXTENSION_readPositionsForVertices[m_EXTENSION_edgeIterator].push_back(distance);

									_UPDATE_SINGLE_VALUES(distance);

									m_EXTENSION_edgeIterator++;
									m_EXTENSION_hasPairedReadRequested=false;
								}else{
									// get the paired end read.
									if(!m_EXTENSION_pairedSequenceRequested){
										m_EXTENSION_pairedSequenceRequested=true;
										VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
										message[0]=annotation.getReadIndex();
										Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_GET_PAIRED_READ,getRank());
										m_outbox.push_back(aMessage);
										m_EXTENSION_pairedSequenceReceived=false;
									}else if(m_EXTENSION_pairedSequenceReceived){
										int expectedFragmentLength=m_EXTENSION_pairedRead.getAverageFragmentLength();
										int expectedDeviation=m_EXTENSION_pairedRead.getStandardDeviation();
										int rank=m_EXTENSION_pairedRead.getRank();
										int id=m_EXTENSION_pairedRead.getId();
										int uniqueReadIdentifier=id*MAX_NUMBER_OF_MPI_PROCESSES+rank;
			
										// it is mandatory for a read to start at 0. at position X on the path.
										if(m_EXTENSION_reads_startingPositionOnContig.count(uniqueReadIdentifier)>0){
											int startingPositionOnPath=m_EXTENSION_reads_startingPositionOnContig[uniqueReadIdentifier];
											int observedFragmentLength=(startPosition-startingPositionOnPath)+m_EXTENSION_receivedLength;
											if(expectedFragmentLength-expectedDeviation<=observedFragmentLength and
											observedFragmentLength <= expectedFragmentLength+expectedDeviation){
											// it matches!
												int theDistance=startPosition-startingPositionOnPath+distance;
												m_EXTENSION_pairedReadPositionsForVertices[m_EXTENSION_edgeIterator].push_back(theDistance);
												if(m_cd->m_CHOOSER_theMaxsPaired.count(m_EXTENSION_edgeIterator)==0){
													m_cd->m_CHOOSER_theMaxsPaired[m_EXTENSION_edgeIterator]=theDistance;
													m_cd->m_CHOOSER_theSumsPaired[m_EXTENSION_edgeIterator]=0;
													m_cd->m_CHOOSER_theNumbersPaired[m_EXTENSION_edgeIterator]=0;
												}
												if(theDistance>m_cd->m_CHOOSER_theMaxsPaired[m_EXTENSION_edgeIterator])
													m_cd->m_CHOOSER_theMaxsPaired[m_EXTENSION_edgeIterator]=theDistance;
												m_cd->m_CHOOSER_theNumbersPaired[m_EXTENSION_edgeIterator]++;
												m_cd->m_CHOOSER_theSumsPaired[m_EXTENSION_edgeIterator]+=theDistance;
											}

										}
										
										// add it anyway as a single-end match too!
										m_EXTENSION_readPositionsForVertices[m_EXTENSION_edgeIterator].push_back(distance);
										m_EXTENSION_hasPairedReadRequested=false;

										_UPDATE_SINGLE_VALUES(distance);
										m_EXTENSION_edgeIterator++;
									}
								}
							}else{
							}
						}else{// no match, too bad.
							m_EXTENSION_edgeIterator++;
							m_EXTENSION_hasPairedReadRequested=false;
						}
					}else{
						m_EXTENSION_readLength_done=false;
						m_EXTENSION_readLength_requested=false;
						m_EXTENSION_readIterator++;
					}
				}
			}else{
				// remove reads that are no longer in-range.
				for(int i=0;i<(int)m_EXTENSION_readsOutOfRange.size();i++){
					m_EXTENSION_readsInRange.erase(m_EXTENSION_readsOutOfRange[i]);
				}
				m_EXTENSION_readsOutOfRange.clear();
				m_EXTENSION_singleEndResolution=true;
				#ifdef SHOW_CHOICE
				if(m_enumerateChoices_outgoingEdges.size()>1){
					cout<<endl;
					cout<<"*****************************************"<<endl;
					cout<<"CurrentVertex="<<idToWord(m_SEEDING_currentVertex,m_wordSize)<<" @"<<m_EXTENSION_extension.size()<<endl;
					cout<<" # ReadsInRange: "<<m_EXTENSION_readsInRange.size()<<endl;
					cout<<m_enumerateChoices_outgoingEdges.size()<<" arcs."<<endl;
					for(int i=0;i<(int)m_enumerateChoices_outgoingEdges.size();i++){
						string vertex=idToWord(m_enumerateChoices_outgoingEdges[i],m_wordSize);
						cout<<endl;
						cout<<"Choice #"<<i+1<<endl;
						cout<<"Vertex: "<<vertex<<endl;
						cout<<"Coverage="<<m_EXTENSION_coverages[i]<<endl;
						cout<<"New letter: "<<vertex[m_wordSize-1]<<endl;
						cout<<"Single-end reads:"<<endl;
						for(int j=0;j<(int)m_EXTENSION_readPositionsForVertices[i].size();j++){
							cout<<" "<<m_EXTENSION_readPositionsForVertices[i][j];
						}
						cout<<endl;
						cout<<"Paired-end reads:"<<endl;
						for(int j=0;j<(int)m_EXTENSION_pairedReadPositionsForVertices[i].size();j++){
							cout<<" "<<m_EXTENSION_pairedReadPositionsForVertices[i][j];
						}
						cout<<endl;
					}
				}
				#endif

				// try to use the coverage to choose.
				// stick around novel minimum coverages
				int i=proceedWithCoverages(m_minimumCoverage/2,m_minimumCoverage);
				if(i>=0){
					return;
				}
				i=proceedWithCoverages(m_minimumCoverage,2*m_minimumCoverage);
				if(i>=0)
					return;

				for(int i=0;i<(int)m_EXTENSION_pairedReadPositionsForVertices.size();i++){
					bool winner=true;
					int coverageI=m_EXTENSION_coverages[i];
					if(coverageI<_MINIMUM_COVERAGE)
						continue;
					if(m_cd->m_CHOOSER_theNumbers[i]==0 or m_cd->m_CHOOSER_theNumbersPaired[i]==0)
						continue;
					for(int j=0;j<(int)m_EXTENSION_pairedReadPositionsForVertices.size();j++){
						if(i==j)
							continue;
						int coverageJ=m_EXTENSION_coverages[j];
						if(coverageJ<_MINIMUM_COVERAGE)
							continue;
						if((m_cd->m_CHOOSER_theMaxsPaired[i] <= __PAIRED_MULTIPLIER*m_cd->m_CHOOSER_theMaxsPaired[j]) or
					 (m_cd->m_CHOOSER_theSumsPaired[i] <= __PAIRED_MULTIPLIER*m_cd->m_CHOOSER_theSumsPaired[j]) or
					 (m_cd->m_CHOOSER_theNumbersPaired[i] <= __PAIRED_MULTIPLIER*m_cd->m_CHOOSER_theNumbersPaired[j]) 
){
							winner=false;
							break;
						}
				
						// if the winner does not have too much coverage.
						if(m_EXTENSION_coverages[i]<m_minimumCoverage and 
					m_cd->m_CHOOSER_theNumbers[i] < m_cd->m_CHOOSER_theNumbers[j]){// make sure that it also has more single-end reads
							winner=false;
							break;
						}

						// too much coverage is sick
						if(coverageI==m_maxCoverage){
						}
					}
					if(winner==true){
						#ifdef SHOW_CHOICE
						if(m_enumerateChoices_outgoingEdges.size()>1){
							cout<<"Choice "<<i+1<<" wins with paired-end reads."<<endl;
						}
						#endif
						m_SEEDING_currentVertex=m_enumerateChoices_outgoingEdges[i];
						m_EXTENSION_choose=true;
						m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
						m_EXTENSION_directVertexDone=false;
						m_EXTENSION_VertexAssembled_requested=false;
						return;
					}
				}

				for(int i=0;i<(int)m_EXTENSION_readPositionsForVertices.size();i++){
					bool winner=true;
					if(m_cd->m_CHOOSER_theMaxs[i]<5)
						winner=false;

					int coverageI=m_EXTENSION_coverages[i];
					if(coverageI<_MINIMUM_COVERAGE)
						continue;
					if(m_cd->m_CHOOSER_theNumbers[i]==0)
						continue;
					for(int j=0;j<(int)m_EXTENSION_readPositionsForVertices.size();j++){
						if(i==j)
							continue;

						if(m_EXTENSION_coverages[j]<_MINIMUM_COVERAGE)
							continue;
						if((m_cd->m_CHOOSER_theMaxs[i] <= __SINGLE_MULTIPLIER*m_cd->m_CHOOSER_theMaxs[j]) 
							or (m_cd->m_CHOOSER_theSums[i] <= __SINGLE_MULTIPLIER*m_cd->m_CHOOSER_theSums[j]) 
							or (m_cd->m_CHOOSER_theNumbers[i] <= __SINGLE_MULTIPLIER*m_cd->m_CHOOSER_theNumbers[j])
							){
							winner=false;
							break;
						}
						if(m_EXTENSION_coverages[i]<m_minimumCoverage/4 and m_EXTENSION_coverages[j]>2*m_minimumCoverage){
							winner=false;
							break;
						}
						// too much coverage is dangereous.
						if(coverageI==m_maxCoverage){
						}

					}
					if(winner==true){
						#ifdef SHOW_CHOICE
						if(m_enumerateChoices_outgoingEdges.size()>1){
							cout<<"Choice "<<i+1<<" wins with single-end reads."<<endl;
						}
						#endif
						m_SEEDING_currentVertex=m_enumerateChoices_outgoingEdges[i];
						m_EXTENSION_choose=true;
						m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
						m_EXTENSION_directVertexDone=false;
						m_EXTENSION_VertexAssembled_requested=false;
						return;
					}
				}

				m_doChoice_tips_Detected=false;
				m_dfsData->m_doChoice_tips_Initiated=false;
				#ifdef SHOW_PROGRESS
				cout<<"Checking tips."<<endl;
				#endif
			}
			return;
		}else if(!m_doChoice_tips_Detected and m_EXTENSION_readsInRange.size()>0){
 			//for each entries in m_enumerateChoices_outgoingEdges, do a dfs of max depth 40.
			//if the reached depth is 40, it is not a tip, otherwise, it is.
			int maxDepth=MAX_DEPTH;
			if(!m_dfsData->m_doChoice_tips_Initiated){
				m_dfsData->m_doChoice_tips_i=0;
				m_dfsData->m_doChoice_tips_newEdges.clear();
				m_dfsData->m_doChoice_tips_dfs_initiated=false;
				m_dfsData->m_doChoice_tips_dfs_done=false;
				m_dfsData->m_doChoice_tips_Initiated=true;
				m_bubbleData->m_BUBBLE_visitedVertices.clear();
				m_bubbleData->m_visitedVertices.clear();
				m_bubbleData->m_BUBBLE_visitedVerticesDepths.clear();
				m_bubbleData->m_coverages.clear();
			}

			if(m_dfsData->m_doChoice_tips_i<(int)m_enumerateChoices_outgoingEdges.size()){
				if(!m_dfsData->m_doChoice_tips_dfs_done){
					depthFirstSearch(m_SEEDING_currentVertex,m_enumerateChoices_outgoingEdges[m_dfsData->m_doChoice_tips_i],maxDepth);
				}else{
					#ifdef SHOW_CHOICE
					cout<<"Choice #"<<m_dfsData->m_doChoice_tips_i+1<<" : visited "<<m_dfsData->m_depthFirstSearchVisitedVertices.size()<<", max depth is "<<m_dfsData->m_depthFirstSearch_maxDepth<<endl;
					#endif
					// keep the edge if it is not a tip.
					if(m_dfsData->m_depthFirstSearch_maxDepth>=TIP_LIMIT){

						// just don't try that strange graph place for now.
						if(m_dfsData->m_depthFirstSearchVisitedVertices.size()==MAX_VERTICES_TO_VISIT){
							/*
							m_doChoice_tips_Detected=true;
							m_bubbleData->m_doChoice_bubbles_Detected=true;
							return;
							*/
						}
						m_dfsData->m_doChoice_tips_newEdges.push_back(m_dfsData->m_doChoice_tips_i);
						m_bubbleData->m_visitedVertices.push_back(m_dfsData->m_depthFirstSearchVisitedVertices);
						// store visited vertices for bubble detection purposes.
						m_bubbleData->m_BUBBLE_visitedVertices.push_back(m_dfsData->m_depthFirstSearchVisitedVertices_vector);
						m_bubbleData->m_coverages.push_back(m_dfsData->m_coverages);
						m_bubbleData->m_BUBBLE_visitedVerticesDepths.push_back(m_dfsData->m_depthFirstSearchVisitedVertices_depths);
					}else{
						#ifdef SHOW_PROGRESS
						cout<<"We have a tip "<<m_dfsData->m_depthFirstSearch_maxDepth<<" LIMIT="<<TIP_LIMIT<<"."<<endl;
						#endif
					}
					m_dfsData->m_doChoice_tips_i++;
					m_dfsData->m_doChoice_tips_dfs_initiated=false;
					m_dfsData->m_doChoice_tips_dfs_done=false;
				}
			}else{
				#ifdef SHOW_PROGRESS
				cout<<m_dfsData->m_doChoice_tips_newEdges.size()<<" new arcs."<<endl;
				#endif
				// we have a winner with tips investigation.
				if(m_dfsData->m_doChoice_tips_newEdges.size()==1 and m_EXTENSION_readsInRange.size()>0 
		and m_EXTENSION_readPositionsForVertices[m_dfsData->m_doChoice_tips_newEdges[0]].size()>0
){
					int readsInFavorOfThis=m_EXTENSION_readPositionsForVertices[m_dfsData->m_doChoice_tips_newEdges[0]].size();
					int coverageAtTheVertexLocation=m_EXTENSION_coverages[m_dfsData->m_doChoice_tips_newEdges[0]];

					// reads are not supportive of this.
					if(readsInFavorOfThis*10<coverageAtTheVertexLocation){
						// no luck..., yet.
						m_doChoice_tips_Detected=true;
						m_bubbleData->m_doChoice_bubbles_Detected=false;
						m_bubbleData->m_doChoice_bubbles_Initiated=false;
						return;
					}

					m_SEEDING_currentVertex=m_enumerateChoices_outgoingEdges[m_dfsData->m_doChoice_tips_newEdges[0]];
					#ifdef SHOW_PROGRESS
					cout<<"We have a win after tip elimination: "<<idToWord(m_SEEDING_currentVertex,m_wordSize)<<endl;
					#endif
					m_EXTENSION_choose=true;
					m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
					m_EXTENSION_directVertexDone=false;
					m_EXTENSION_VertexAssembled_requested=false;
					return;
				}else{
					// no luck..., yet.
					m_doChoice_tips_Detected=true;
					m_bubbleData->m_doChoice_bubbles_Detected=false;
					m_bubbleData->m_doChoice_bubbles_Initiated=false;
				}
			}
			return;
		// bubbles detection aims polymorphisms and homopolymers stretches.
		}else if(!m_bubbleData->m_doChoice_bubbles_Detected and m_EXTENSION_readsInRange.size()>0){
			BubbleTool tool;
			bool isGenuineBubble=tool.isGenuineBubble(m_SEEDING_currentVertex,&m_bubbleData->m_BUBBLE_visitedVertices);

			// support indels of 1 as well as mismatch polymorphisms.
			if(isGenuineBubble){
				cout<<"Forcing next choice "<<endl;
				m_SEEDING_currentVertex=m_enumerateChoices_outgoingEdges[m_dfsData->m_doChoice_tips_newEdges[0]];
				m_EXTENSION_choose=true;
				m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
				m_EXTENSION_directVertexDone=false;
				m_EXTENSION_VertexAssembled_requested=false;
			}
			m_bubbleData->m_doChoice_bubbles_Detected=true;
			return;
		}

		// no choice possible...
		// do it for the lulz
		if(!m_EXTENSION_complementedSeed){
			#ifdef SHOW_PROGRESS
			cout<<"Switching to reverse complement."<<endl;
			#endif
			m_EXTENSION_complementedSeed=true;
			vector<VERTEX_TYPE> complementedSeed;
			for(int i=m_EXTENSION_extension.size()-1;i>=0;i--){
				complementedSeed.push_back(complementVertex(m_EXTENSION_extension[i],m_wordSize,m_colorSpaceMode));
			}
			m_EXTENSION_currentPosition=0;
			m_EXTENSION_currentSeed=complementedSeed;
			m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
			m_SEEDING_currentVertex=m_EXTENSION_currentSeed[m_EXTENSION_currentPosition];
			m_EXTENSION_extension.clear();
			m_EXTENSION_usedReads.clear();
			m_EXTENSION_directVertexDone=false;
			m_EXTENSION_VertexAssembled_requested=false;
			m_EXTENSION_reads_startingPositionOnContig.clear();
			m_EXTENSION_readsInRange.clear();
		}else{
			if(m_EXTENSION_extension.size()>=100){
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<getRank()<<" stores an extension, "<<m_EXTENSION_extension.size()<<" vertices."<<endl;
				#endif
				m_EXTENSION_contigs.push_back(m_EXTENSION_extension);

				int id=m_EXTENSION_currentSeedIndex*MAX_NUMBER_OF_MPI_PROCESSES+getRank();
				m_EXTENSION_identifiers.push_back(id);
			}
			m_EXTENSION_currentSeedIndex++;
			m_EXTENSION_currentPosition=0;
			if(m_EXTENSION_currentSeedIndex<(int)m_SEEDING_seeds.size()){
				m_EXTENSION_currentSeed=m_SEEDING_seeds[m_EXTENSION_currentSeedIndex];
				m_SEEDING_currentVertex=m_EXTENSION_currentSeed[m_EXTENSION_currentPosition];
			}
			m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
			m_EXTENSION_extension.clear();
			m_EXTENSION_reads_startingPositionOnContig.clear();
			m_EXTENSION_readsInRange.clear();
			m_EXTENSION_usedReads.clear();
			m_EXTENSION_directVertexDone=false;
			m_EXTENSION_complementedSeed=false;
			m_EXTENSION_VertexAssembled_requested=false;
		}
	}
}

/*
 * do a depth first search with max depth of maxDepth;
 */
void Machine::depthFirstSearch(VERTEX_TYPE root,VERTEX_TYPE a,int maxDepth){
	if(!m_dfsData->m_doChoice_tips_dfs_initiated){
		m_dfsData->m_depthFirstSearchVisitedVertices.clear();
		m_dfsData->m_depthFirstSearchVisitedVertices_vector.clear();
		m_dfsData->m_depthFirstSearchVisitedVertices_depths.clear();
		while(m_dfsData->m_depthFirstSearchVerticesToVisit.size()>0)
			m_dfsData->m_depthFirstSearchVerticesToVisit.pop();
		while(m_dfsData->m_depthFirstSearchDepths.size()>0)
			m_dfsData->m_depthFirstSearchDepths.pop();

		m_dfsData->m_depthFirstSearchVerticesToVisit.push(a);
		m_dfsData->m_depthFirstSearchVisitedVertices.insert(a);
		m_dfsData->m_depthFirstSearchDepths.push(0);
		m_dfsData->m_depthFirstSearch_maxDepth=0;
		m_dfsData->m_doChoice_tips_dfs_initiated=true;
		m_dfsData->m_doChoice_tips_dfs_done=false;
		m_dfsData->m_coverages.clear();
		m_SEEDING_edgesRequested=false;
		m_SEEDING_vertexCoverageRequested=false;
		#ifdef SHOW_MINI_GRAPH
		cout<<"<MiniGraph>"<<endl;
		cout<<idToWord(root,m_wordSize)<<" -> "<<idToWord(a,m_wordSize)<<endl;
		#endif
	}
	if(m_dfsData->m_depthFirstSearchVerticesToVisit.size()>0){
		VERTEX_TYPE vertexToVisit=m_dfsData->m_depthFirstSearchVerticesToVisit.top();
		if(!m_SEEDING_vertexCoverageRequested){
			m_SEEDING_vertexCoverageRequested=true;
			m_SEEDING_vertexCoverageReceived=false;
			
			VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
			message[0]=vertexToVisit;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_REQUEST_VERTEX_COVERAGE,getRank());
			m_outbox.push_back(aMessage);
		}else if(m_SEEDING_vertexCoverageReceived){
			if(!m_SEEDING_edgesRequested){
				m_dfsData->m_coverages[vertexToVisit]=m_SEEDING_receivedVertexCoverage;
				if(m_SEEDING_receivedVertexCoverage>1){
					m_dfsData->m_depthFirstSearchVisitedVertices.insert(vertexToVisit);
				}else{
					// don't visit it.
					m_dfsData->m_depthFirstSearchVerticesToVisit.pop();
					m_dfsData->m_depthFirstSearchDepths.pop();
					m_SEEDING_edgesRequested=false;
					m_SEEDING_vertexCoverageRequested=false;
					return;
				}
				int theDepth=m_dfsData->m_depthFirstSearchDepths.top();
				if(m_dfsData->m_depthFirstSearchVisitedVertices.size()>=MAX_VERTICES_TO_VISIT){
					// quit this strange place.
	
					m_dfsData->m_doChoice_tips_dfs_done=true;
					#ifdef SHOW_TIP_LOST
					cout<<"Exiting, I am lost. "<<m_dfsData->m_depthFirstSearchVisitedVertices.size()<<""<<endl;
					#endif
					return;
				}
				// too far away.
				if(theDepth> m_dfsData->m_depthFirstSearch_maxDepth){
					m_dfsData->m_depthFirstSearch_maxDepth=theDepth;
				}
			
				// visit the vertex, and ask next edges.
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=vertexToVisit;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(vertexToVisit),TAG_REQUEST_VERTEX_OUTGOING_EDGES,getRank());
				m_outbox.push_back(aMessage);
				m_SEEDING_edgesRequested=true;
				m_SEEDING_edgesReceived=false;
			}else if(m_SEEDING_edgesReceived){
				VERTEX_TYPE vertexToVisit=m_dfsData->m_depthFirstSearchVerticesToVisit.top();
				int theDepth=m_dfsData->m_depthFirstSearchDepths.top();
				#ifdef DEBUG
				assert(theDepth>=0);
				assert(theDepth<=maxDepth);
				#endif
				int newDepth=theDepth+1;

				m_dfsData->m_depthFirstSearchVerticesToVisit.pop();
				m_dfsData->m_depthFirstSearchDepths.pop();


				#ifdef SHOW_MINI_GRAPH
				if(m_dfsData->m_coverages[vertexToVisit]>=m_minimumCoverage/2){
					string b=idToWord(vertexToVisit,m_wordSize);
					cout<<b<<" [label=\""<<b<<" "<<m_SEEDING_receivedVertexCoverage<<"\" ]"<<endl;
				}
				#endif

				for(int i=0;i<(int)m_SEEDING_receivedOutgoingEdges.size();i++){
					VERTEX_TYPE nextVertex=m_SEEDING_receivedOutgoingEdges[i];
					if(m_dfsData->m_depthFirstSearchVisitedVertices.count(nextVertex)>0)
						continue;
					if(newDepth>maxDepth)
						continue;
					m_dfsData->m_depthFirstSearchVerticesToVisit.push(nextVertex);
					m_dfsData->m_depthFirstSearchDepths.push(newDepth);
					if(m_dfsData->m_coverages[vertexToVisit]>=m_minimumCoverage/2){
						m_dfsData->m_depthFirstSearchVisitedVertices_vector.push_back(vertexToVisit);
						m_dfsData->m_depthFirstSearchVisitedVertices_vector.push_back(nextVertex);
						m_dfsData->m_depthFirstSearchVisitedVertices_depths.push_back(newDepth);

						#ifdef SHOW_MINI_GRAPH
						cout<<idToWord(vertexToVisit,m_wordSize)<<" -> "<<idToWord(nextVertex,m_wordSize)<<endl;
						#endif
					}
				}
				m_SEEDING_edgesRequested=false;
				m_SEEDING_vertexCoverageRequested=false;
			}
		}
	}else{
		m_dfsData->m_doChoice_tips_dfs_done=true;
		#ifdef SHOW_MINI_GRAPH
		cout<<"</MiniGraph>"<<endl;
		#endif
	}
}


void Machine::checkIfCurrentVertexIsAssembled(){
	if(!m_EXTENSION_directVertexDone){
		if(!m_EXTENSION_VertexAssembled_requested){
			if(m_EXTENSION_currentSeedIndex%10==0 and m_EXTENSION_currentPosition==0 and m_last_value!=m_EXTENSION_currentSeedIndex){
				m_last_value=m_EXTENSION_currentSeedIndex;
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<getRank()<<": extending seeds "<<m_EXTENSION_currentSeedIndex<<"/"<<m_SEEDING_seeds.size()<<endl;
				#endif
			}
			m_EXTENSION_VertexAssembled_requested=true;
			VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
			message[0]=(VERTEX_TYPE)m_SEEDING_currentVertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(m_SEEDING_currentVertex),TAG_ASK_IS_ASSEMBLED,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_VertexAssembled_received=false;
		}else if(m_EXTENSION_VertexAssembled_received){
			m_EXTENSION_reverseVertexDone=false;
			m_EXTENSION_directVertexDone=true;
			m_EXTENSION_VertexMarkAssembled_requested=false;
			m_SEEDING_vertexCoverageRequested=false;
			m_EXTENSION_VertexAssembled_requested=false;
			if(m_EXTENSION_vertexIsAssembledResult){
				m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
				m_EXTENSION_markedCurrentVertexAsAssembled=false;
				m_EXTENSION_directVertexDone=false;
			}else{
			}
		}else{
		}
	}else if(!m_EXTENSION_reverseVertexDone){
		if(!m_EXTENSION_VertexAssembled_requested){
			m_EXTENSION_VertexAssembled_requested=true;
			VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
			message[0]=(VERTEX_TYPE)complementVertex(m_SEEDING_currentVertex,m_wordSize,m_colorSpaceMode);
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_ASK_IS_ASSEMBLED,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_VertexAssembled_received=false;
		}else if(m_EXTENSION_VertexAssembled_received){
			m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
			m_EXTENSION_markedCurrentVertexAsAssembled=false;
			m_EXTENSION_directVertexDone=false;
		}else{
		}
	}
}

void Machine::markCurrentVertexAsAssembled(){
	if(!m_EXTENSION_directVertexDone){
		if(!m_SEEDING_vertexCoverageRequested){
			m_SEEDING_vertexCoverageRequested=true;
			m_SEEDING_vertexCoverageReceived=false;
			
			VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
			message[0]=m_SEEDING_currentVertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),TAG_REQUEST_VERTEX_COVERAGE,getRank());
			m_outbox.push_back(aMessage);
		}else if(m_SEEDING_vertexCoverageReceived){
			if(!m_EXTENSION_VertexMarkAssembled_requested){
				m_EXTENSION_extension.push_back(m_SEEDING_currentVertex);
				// save wave progress.
	
				int waveId=m_EXTENSION_currentSeedIndex*MAX_NUMBER_OF_MPI_PROCESSES+getRank();
				int progression=m_EXTENSION_extension.size()-1;
			

				m_EXTENSION_VertexMarkAssembled_requested=true;
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(3*sizeof(VERTEX_TYPE));
				message[0]=(VERTEX_TYPE)m_SEEDING_currentVertex;
				message[1]=waveId;
				message[2]=progression;
				Message aMessage(message,3,MPI_UNSIGNED_LONG_LONG,vertexRank(m_SEEDING_currentVertex),TAG_SAVE_WAVE_PROGRESSION,getRank());
				m_outbox.push_back(aMessage);
				m_EXTENSION_reverseVertexDone=true;
				m_EXTENSION_reads_requested=false;

			// get the reads starting at that position.
			}else if(!m_EXTENSION_reads_requested){
				m_EXTENSION_reads_requested=true;
				m_EXTENSION_reads_received=false;
				VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator.allocate(1*sizeof(VERTEX_TYPE));
				message[0]=(VERTEX_TYPE)m_SEEDING_currentVertex;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(m_SEEDING_currentVertex),TAG_REQUEST_READS,getRank());
				m_outbox.push_back(aMessage);
			
			}else if(m_EXTENSION_reads_received){
				for(int i=0;i<(int)m_EXTENSION_receivedReads.size();i++){
					int uniqueId=m_EXTENSION_receivedReads[i].getUniqueId();
					// check that the complete sequence of m_SEEDING_currentVertex correspond to
					// the one of the start of the read on the good strand.
					// this is important when USE_DISTANT_SEGMENTS_GRAPH is set.

					if(m_EXTENSION_usedReads.count(uniqueId)==0 ){
						// use all reads available.
						if(true or m_SEEDING_receivedVertexCoverage<3*m_peakCoverage){
							m_EXTENSION_usedReads.insert(uniqueId);
							m_EXTENSION_reads_startingPositionOnContig[uniqueId]=m_EXTENSION_extension.size()-1;
							m_EXTENSION_readsInRange.insert(m_EXTENSION_receivedReads[i]);
							#ifdef DEBUG
							assert(m_EXTENSION_readsInRange.count(m_EXTENSION_receivedReads[i])>0);
							#endif
						}else{
							cout<<"Repeated vertex."<<endl;
						}
					}
				}
				m_EXTENSION_directVertexDone=true;
				m_EXTENSION_VertexMarkAssembled_requested=false;
				m_EXTENSION_enumerateChoices=false;
				m_SEEDING_edgesRequested=false;
				m_EXTENSION_markedCurrentVertexAsAssembled=true;
			}
		}
	}
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
