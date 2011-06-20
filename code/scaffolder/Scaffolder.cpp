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

#include <iostream>
#include <assembler/ReadFetcher.h>
#include <scaffolder/Scaffolder.h>
#include <communication/Message.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>
using namespace std;

void Scaffolder::addMasterLink(vector<uint64_t>*a){
	m_masterLinks.push_back(*a);
}

void Scaffolder::addMasterContig(uint64_t name,int length){
	m_masterContigs.push_back(name);
	m_masterLengths.push_back(length);
}

void Scaffolder::solve(){
	vector<vector<uint64_t> > megaLinks;

/*
 *  The value of the minimum number of raw links is completely arbitrary.
 *  However, it reduces significantly the number of scaffolds.
 *  Therefore, its usage is warranted.
 */
	int minimumNumberOfRawLinks=3;
	map<uint64_t,map<char,map<uint64_t,map<char,vector<int> > > > > keys;
	for(int i=0;i<(int)m_masterLinks.size();i++){
		uint64_t leftContig=m_masterLinks[i][0];
		char leftStrand=m_masterLinks[i][1];
		uint64_t rightContig=m_masterLinks[i][2];
		char rightStrand=m_masterLinks[i][3];
		int average=m_masterLinks[i][4];
		int number=m_masterLinks[i][5];
		if(number<minimumNumberOfRawLinks)
			continue;
		keys[leftContig][leftStrand][rightContig][rightStrand].push_back(average);
		keys[leftContig][leftStrand][rightContig][rightStrand].push_back(number);
	}

	map<uint64_t,map<uint64_t,int> > validCounts;

	ostringstream linkFile;
	linkFile<<m_parameters->getPrefix()<<".ScaffoldLinks.txt";
	ofstream f(linkFile.str().c_str());

	for(map<uint64_t,map<char,map<uint64_t,map<char,vector<int> > > > >::iterator i=
		keys.begin();i!=keys.end();i++){
		uint64_t leftContig=i->first;
		for(map<char,map<uint64_t,map<char,vector<int> > > >::iterator j=i->second.begin();
			j!=i->second.end();j++){
			char leftStrand=j->first;
			for(map<uint64_t,map<char,vector<int> > >::iterator k=j->second.begin();
				k!=j->second.end();k++){
				uint64_t rightContig=k->first;
				for(map<char,vector<int> >::iterator l=k->second.begin();
					l!=k->second.end();l++){
					char rightStrand=l->first;
					int sum=0;
					int n=0;
					int pos=0;
					for(vector<int>::iterator m=l->second.begin();m!=l->second.end();m++){
						if(pos%2==0){
							sum+=*m;
							n++;
						}
						pos++;
					}
					if(n==2){
						int average=sum/n;
						f<<"contig-"<<leftContig<<"\t"<<leftStrand<<"\tcontig-"<<rightContig<<"\t"<<rightStrand<<"\t"<<average<<endl;
						vector<uint64_t> megaLink;
						megaLink.push_back(leftContig);
						megaLink.push_back(leftStrand);
						megaLink.push_back(rightContig);
						megaLink.push_back(rightStrand);
						megaLink.push_back(average);
						megaLinks.push_back(megaLink);
						validCounts[leftContig][rightContig]++;
						validCounts[rightContig][leftContig]++;
					}
				}
			}
		}
	}
	f.close();
	
	// create the graph
	set<uint64_t> vertices;
	map<uint64_t,map<char,vector<vector<uint64_t> > > > parents;
	map<uint64_t,map<char,vector<vector<uint64_t> > > > children;
	for(int i=0;i<(int)megaLinks.size();i++){
		uint64_t leftContig=megaLinks[i][0];
		char leftStrand=megaLinks[i][1];
		uint64_t rightContig=megaLinks[i][2];
		char rightStrand=megaLinks[i][3];
		int distance=megaLinks[i][4];
		char otherLeftStrand='F';
		vertices.insert(leftContig);
		vertices.insert(rightContig);
		if(leftStrand=='F')
			otherLeftStrand='R';
		char otherRightStrand='F';
		if(rightStrand=='F')
			otherRightStrand='R';
		children[leftContig][leftStrand].push_back(megaLinks[i]);
		parents[rightContig][rightStrand].push_back(megaLinks[i]);
		vector<uint64_t> reverseLink;
		reverseLink.push_back(rightContig);
		reverseLink.push_back(otherRightStrand);
		reverseLink.push_back(leftContig);
		reverseLink.push_back(otherLeftStrand);
		reverseLink.push_back(distance);
		children[rightContig][otherRightStrand].push_back(reverseLink);
		parents[leftContig][otherLeftStrand].push_back(reverseLink);
	}

	// add colors to the graph
	map<uint64_t,int> colors;
	map<int,vector<uint64_t> > colorMap;
	int i=0;
	for(set<uint64_t>::iterator j=vertices.begin();j!=vertices.end();j++){
		colors[*j]=i;
		colorMap[i].push_back(*j);
		i++;
	}
	
	// write contig list
	ostringstream contigList;
	contigList<<m_parameters->getPrefix()<<".ContigLengths.txt";
	ofstream f2(contigList.str().c_str());
	m_numberOfContigsWithAtLeastThreshold=0;
	m_totalContigLength=0;
	m_totalContigLengthWithThreshold=0;
	m_numberOfContigs=m_masterContigs.size();
	for(int i=0;i<(int)m_masterContigs.size();i++){
		int length=m_masterLengths[i]+m_parameters->getWordSize()-1;
		if(length>=m_parameters->getLargeContigThreshold()){
			m_numberOfContigsWithAtLeastThreshold++;
			m_totalContigLengthWithThreshold+=length;
		}
		f2<<"contig-"<<m_masterContigs[i]<<"\t"<<length<<endl;
		m_totalContigLength+=length;
	}
	f2.close();

	// do some color merging.
	for(set<uint64_t>::iterator j=vertices.begin();j!=vertices.end();j++){
		uint64_t vertex=*j;
		char state='F';
		if(children.count(vertex)>0&&children[vertex].count(state)>0
			&&children[vertex][state].size()==1){
			uint64_t childVertex=children[vertex][state][0][2];
			char childState=children[vertex][state][0][3];
			if(parents[childVertex][childState].size()==1
			&&validCounts[vertex][childVertex]==1){
				int currentColor=colors[vertex];
				int childColor=colors[childVertex];
				if(currentColor!=childColor){
					for(int i=0;i<(int)colorMap[childColor].size();i++){
						uint64_t otherVertex=colorMap[childColor][i];
						colors[otherVertex]=currentColor;
						colorMap[currentColor].push_back(otherVertex);
					}
					colorMap.erase(childColor);
				}
			}
		}
		state='R';
		if(children.count(vertex)>0&&children[vertex].count(state)>0
			&&children[vertex][state].size()==1){
			uint64_t childVertex=children[vertex][state][0][2];
			char childState=children[vertex][state][0][3];
			if(parents[childVertex][childState].size()==1
			&& validCounts[vertex][childVertex]==1){
				int currentColor=colors[vertex];
				int childColor=colors[childVertex];
				if(currentColor!=childColor){
					for(int i=0;i<(int)colorMap[childColor].size();i++){
						uint64_t otherVertex=colorMap[childColor][i];
						colors[otherVertex]=currentColor;
						colorMap[currentColor].push_back(otherVertex);
					}
					colorMap.erase(childColor);
				}
			}
		}
	}

	// extract scaffolds
	set<int>completedColours;
	for(set<uint64_t>::iterator j=vertices.begin();j!=vertices.end();j++){
		uint64_t vertex=*j;
		extractScaffolds('F',&colors,vertex,&parents,&children,&completedColours);
		extractScaffolds('R',&colors,vertex,&parents,&children,&completedColours);
	}

	// add unscaffolded stuff.
	for(int i=0;i<(int)m_masterContigs.size();i++){
		uint64_t contig=m_masterContigs[i];
		if(colors.count(contig)==0){
			vector<uint64_t> contigs;
			vector<char> strands;
			contigs.push_back(m_masterContigs[i]);
			strands.push_back('F');
			m_scaffoldContigs.push_back(contigs);
			m_scaffoldStrands.push_back(strands);
		}
	}

	for(int i=0;i<(int)m_masterLengths.size();i++){
		m_contigLengths[m_masterContigs[i]]=m_masterLengths[i];
	}

	// write scaffold list
	ostringstream scaffoldList;
	scaffoldList<<m_parameters->getPrefix()<<".ScaffoldComponents.txt";
	ostringstream scaffoldLengths;
	m_numberOfScaffoldsWithThreshold=0;
	m_totalScaffoldLengthWithThreshold=0;
	scaffoldLengths<<m_parameters->getPrefix()<<".ScaffoldLengths.txt";
	ofstream f3(scaffoldLengths.str().c_str());
	ofstream f4(scaffoldList.str().c_str());
	m_totalScaffoldLength=0;
	m_numberOfScaffolds=m_scaffoldContigs.size();
	for(int i=0;i<(int)m_scaffoldContigs.size();i++){
		int scaffoldName=i;
		int length=0;
		for(int j=0;j<(int)m_scaffoldContigs[i].size();j++){
			uint64_t contigName=m_scaffoldContigs[i][j];
			char contigStrand=m_scaffoldStrands[i][j];
			int theLength=m_contigLengths[contigName]+m_parameters->getWordSize()-1;
			f4<<"scaffold-"<<scaffoldName<<"\t"<<"contig-"<<contigName<<"\t"<<contigStrand<<"\t"<<theLength<<endl;
			length+=theLength;
			if(j!=(int)m_scaffoldContigs[i].size()-1){
				int theLength=m_scaffoldGaps[i][j];
				f4<<"scaffold-"<<scaffoldName<<"\tgap\t-\t"<<theLength<<endl;
				length+=theLength;
			}
		}
		f3<<"scaffold-"<<scaffoldName<<"\t"<<length<<endl;
		m_totalScaffoldLength+=length;
		f4<<endl;
		if(length>=m_parameters->getLargeContigThreshold()){
			m_numberOfScaffoldsWithThreshold++;
			m_totalScaffoldLengthWithThreshold+=length;
		}
	}
	f2.close();
	f3.close();
	ostringstream outputStat;
	outputStat<<m_parameters->getPrefix()<<".OutputNumbers.txt";
	ofstream f5(outputStat.str().c_str());

	f5<<"Number of contigs:\t"<<m_numberOfContigs<<endl;
	f5<<"Total length of contigs:\t"<<m_totalContigLength<<endl;
	f5<<"Number of contigs >= "<<m_parameters->getLargeContigThreshold()<<" nt:\t"<<m_numberOfContigsWithAtLeastThreshold<<endl;
	f5<<"Total length of contigs >= "<<m_parameters->getLargeContigThreshold()<<" nt:\t"<<m_totalContigLengthWithThreshold<<endl;
	f5<<"Number of scaffolds:\t"<<m_numberOfScaffolds<<endl;
	f5<<"Total length of scaffolds:\t"<<m_totalScaffoldLength<<endl;
	f5<<"Number of scaffolds >= "<<m_parameters->getLargeContigThreshold()<<" nt:\t"<<m_numberOfScaffoldsWithThreshold<<endl;
	f5<<"Total length of scaffolds >= "<<m_parameters->getLargeContigThreshold()<<":\t"<<m_totalScaffoldLengthWithThreshold<<endl;

	f5.close();
}

void Scaffolder::printFinalMessage(){
	cout<<"Number of contigs:\t"<<m_numberOfContigs<<endl;
	cout<<"Total length of contigs:\t"<<m_totalContigLength<<endl;
	cout<<"Number of contigs >= "<<m_parameters->getLargeContigThreshold()<<" nt:\t"<<m_numberOfContigsWithAtLeastThreshold<<endl;
	cout<<"Total length of contigs >= "<<m_parameters->getLargeContigThreshold()<<" nt:\t"<<m_totalContigLengthWithThreshold<<endl;
	cout<<"Number of scaffolds:\t"<<m_numberOfScaffolds<<endl;
	cout<<"Total length of scaffolds:\t"<<m_totalScaffoldLength<<endl;
	cout<<"Number of scaffolds >= "<<m_parameters->getLargeContigThreshold()<<" nt:\t"<<m_numberOfScaffoldsWithThreshold<<endl;
	cout<<"Total length of scaffolds >= "<<m_parameters->getLargeContigThreshold()<<":\t"<<m_totalScaffoldLengthWithThreshold<<endl;
}

void Scaffolder::constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,Parameters*parameters,
	int*slaveMode,VirtualCommunicator*vc){
	m_slave_mode=slaveMode;
	m_virtualCommunicator=vc;
	m_outbox=outbox;
	m_inbox=inbox;
	m_outboxAllocator=outboxAllocator;
	m_parameters=parameters;
	m_initialised=false;
	m_workerId=0;
}

void Scaffolder::extractScaffolds(char state,map<uint64_t,int>*colors,uint64_t vertex,
	map<uint64_t,map<char,vector<vector<uint64_t> > > >*parents,
	map<uint64_t,map<char,vector<vector<uint64_t> > > >*children,set<int>*completedColours){
	vector<uint64_t> contigs;
	vector<char> strands;
	vector<int> gaps;
	bool skip=false;
	int currentColor=(*colors)[vertex];
	if((*completedColours).count(currentColor)>0)
		return;

	if((*parents).count(vertex)>0&&(*parents)[vertex].count(state)>0){
		for(int i=0;i<(int)(*parents)[vertex][state].size();i++){
			#ifdef ASSERT
			assert(0<(*parents)[vertex][state][i].size());
			#endif
			uint64_t parent=(*parents)[vertex][state][i][0];
			int parentColor=(*colors)[parent];
			if(parentColor==currentColor){
				skip=true;
				break;
			}
		}
	}
	if(skip)
		return;

	(*completedColours).insert(currentColor);
	bool done=false;
	while(!done){
		contigs.push_back(vertex);
		strands.push_back(state);
		if((*children).count(vertex)>0&&(*children)[vertex].count(state)>0){
			bool found=false;
			for(int i=0;i<(int)(*children)[vertex][state].size();i++){
				#ifdef ASSERT
				assert(2<(*children)[vertex][state][i].size());
				#endif
				uint64_t childVertex=(*children)[vertex][state][i][2];
				int childColor=(*colors)[childVertex];
				if(childColor==currentColor){
					#ifdef ASSERT
					assert(3<(*children)[vertex][state][i].size());
					#endif
					char childState=(*children)[vertex][state][i][3];
					#ifdef ASSERT
					assert(4<(*children)[vertex][state][i].size());
					#endif
					int gap=(*children)[vertex][state][i][4];
					gaps.push_back(gap);

					vertex=childVertex;
					state=childState;
					found=true;
					break;
				}
			}
			if(!found){
				done=true;
			}
		}else{
			done=true;
		}
	}
	m_scaffoldContigs.push_back(contigs);
	m_scaffoldStrands.push_back(strands);
	m_scaffoldGaps.push_back(gaps);
	return;
}

void Scaffolder::run(){
	if(!m_initialised){
		m_initialised=true;
		m_ready=true;
		m_contigId=0;
		m_positionOnContig=0;
		m_forwardDone=false;
		m_coverageRequested=false;
	}

	m_virtualCommunicator->forceFlush();
	m_virtualCommunicator->processInbox(&m_activeWorkers);
	m_activeWorkers.clear();

	if(m_contigId<(int)m_contigs.size()){
		processContig();
	}else{
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_I_FINISHED_SCAFFOLDING,
			m_parameters->getRank());
		m_outbox->push_back(aMessage);
		(*m_slave_mode)=RAY_SLAVE_MODE_DO_NOTHING;
	}
}

void Scaffolder::addContig(uint64_t name,vector<Kmer>*vertices){
	m_contigNames.push_back(name);
	m_contigs.push_back(*vertices);
}

void Scaffolder::processContig(){
	if(m_positionOnContig<(int)m_contigs[m_contigId].size()){
		processContigPosition();
	}else if(!m_summaryPerformed){
		performSummary();
	}else if(!m_summarySent){
		sendSummary();
	}else if(!m_sentContigMeta){
		sendContigInfo();
	}else{
		m_contigId++;
		m_positionOnContig=0;
	}
}

void Scaffolder::sendContigInfo(){
	if(!m_sentContigInfo){
		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		message[0]=m_contigNames[m_contigId];
		message[1]=m_contigs[m_contigId].size();
		Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,
			MASTER_RANK,RAY_MPI_TAG_CONTIG_INFO,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_sentContigInfo=true;
	}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
		m_virtualCommunicator->getMessageResponseElements(m_workerId);
		m_sentContigMeta=true;
	}
}

void Scaffolder::sendSummary(){
	if(m_summaryIterator<(int)m_summary.size()){
		if(!m_entrySent){
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
			for(int i=0;i<6;i++){
				message[i]=m_summary[m_summaryIterator][i];
			}
			Message aMessage(message,6,MPI_UNSIGNED_LONG_LONG,
				MASTER_RANK,RAY_MPI_TAG_SCAFFOLDING_LINKS,m_parameters->getRank());
			m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
			m_entrySent=true;
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
			m_virtualCommunicator->getMessageResponseElements(m_workerId);
			m_summaryIterator++;
			m_entrySent=false;
		}
	}else{
		m_summarySent=true;
		m_sentContigMeta=false;
		m_sentContigInfo=false;
	}
}

void Scaffolder::performSummary(){
	m_summary.clear();
	m_summaryIterator=0;
	for(map<uint64_t,map<char,map<uint64_t,map<char,vector<int> > > > >::iterator i=
		m_scaffoldingSummary.begin();i!=m_scaffoldingSummary.end();i++){
		uint64_t leftContig=i->first;
		for(map<char,map<uint64_t,map<char,vector<int> > > >::iterator j=i->second.begin();
			j!=i->second.end();j++){
			char leftStrand=j->first;
			for(map<uint64_t,map<char,vector<int> > >::iterator k=j->second.begin();
				k!=j->second.end();k++){
				uint64_t rightContig=k->first;
				for(map<char,vector<int> >::iterator l=k->second.begin();
					l!=k->second.end();l++){
					char rightStrand=l->first;
					int sum=0;
					int n=0;
					for(vector<int>::iterator m=l->second.begin();m!=l->second.end();m++){
						int distance=*m;
						sum+=distance;
						n++;
					}
					int average=sum/n;

					vector<uint64_t> entry;
					entry.push_back(leftContig);
					entry.push_back(leftStrand);
					entry.push_back(rightContig);
					entry.push_back(rightStrand);
					entry.push_back(average);
					entry.push_back(n);
					m_summary.push_back(entry);
				}
			}
		}
	}
	m_summaryPerformed=true;
	m_summarySent=false;
	m_entrySent=false;
}

void Scaffolder::processContigPosition(){
	#ifdef ASSERT
	assert(m_contigId<(int)m_contigs.size());
	assert(m_positionOnContig<(int)m_contigs[m_contigId].size());
	#endif

	Kmer vertex=m_contigs[m_contigId][m_positionOnContig];
	#ifdef ASSERT
	assert(m_parameters!=NULL);
	#endif
	if(!m_forwardDone){
		processVertex(vertex);
	}else if(!m_reverseDone){
		// get the coverage
		// if < maxCoverage
		// 	get read markers
		// 	for each read marker
		// 		if it is paired
		// 			get its pair
		// 				get the vertex for the opposite strand of the first read
		// 				get the coverage of this vertex
		// 				if < maxCoverage
		// 					get the paths that goes on them
		// 					print the linking information

		m_reverseDone=true;
	}else{
		m_positionOnContig++;
		m_forwardDone=false;
		m_coverageRequested=false;
	}
}

void Scaffolder::processVertex(Kmer vertex){
	// get the coverage
	// if < maxCoverage
	// 	get read markers
	// 	for each read marker
	// 		if it is paired
	// 			get its pair
	// 				get the vertex for the opposite strand of the first read
	// 				get the coverage of this vertex
	// 				if < maxCoverage
	// 					get the paths that goes on them
	// 					print the linking information
	if(!m_coverageRequested){
		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		int bufferPosition=0;
		vertex.pack(buffer,&bufferPosition);
		Message aMessage(buffer,bufferPosition,MPI_UNSIGNED_LONG_LONG,
			m_parameters->_vertexRank(&vertex),RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_coverageRequested=true;
		m_coverageReceived=false;
		if(m_positionOnContig==0){
			m_scaffoldingSummary.clear();
			m_summaryPerformed=false;
		}
		if(m_positionOnContig==(int)m_contigs[m_contigId].size()-1){
			printf("Rank %i: gathering scaffold links [%i/%i] [%i/%i] (completed)\n",m_parameters->getRank(),
				m_contigId+1,(int)m_contigs.size(),
				m_positionOnContig+1,(int)m_contigs[m_contigId].size());
		}else if(m_positionOnContig%10000==0){
			printf("Rank %i: gathering scaffold links [%i/%i] [%i/%i]\n",m_parameters->getRank(),
				m_contigId+1,(int)m_contigs.size(),
				m_positionOnContig+1,(int)m_contigs[m_contigId].size());
		}
	}else if(!m_coverageReceived
		&&m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<uint64_t>answer=m_virtualCommunicator->getMessageResponseElements(m_workerId);
		#ifdef ASSERT
		assert(0<answer.size());
		#endif
		m_receivedCoverage=answer[0];
		m_coverageReceived=true;
		m_initialisedFetcher=false;
	}else if(m_coverageReceived){
		if(m_receivedCoverage<m_parameters->getPeakCoverage()){
			if(!m_initialisedFetcher){
				m_readFetcher.constructor(&vertex,m_outboxAllocator,m_inbox,
				m_outbox,m_parameters,m_virtualCommunicator,m_workerId);
				m_readAnnotationId=0;
				m_initialisedFetcher=true;
				m_hasPairRequested=false;
			}else if(!m_readFetcher.isDone()){
				m_readFetcher.work();
			}else{
				processAnnotations();
			}
		}else{
			m_forwardDone=true;
			m_reverseDone=false;
		}
	}
}

void Scaffolder::processAnnotations(){
	if(m_readAnnotationId<(int)m_readFetcher.getResult()->size()){
		processAnnotation();
	}else{
		m_forwardDone=true;
		m_reverseDone=false;
	}
}

void Scaffolder::processAnnotation(){
	// if is paired
	// 	get the forward and the reverse markers
	// 	get the coverage of the forward vertex
	// 	if < maxCoverage
	//	 	get the Direction
	//	 	if only 1 Direction
	//	 		if contig is not self
	//	 			get its length
	//	 			print link information
	//
	// 	get the coverage of the reverse vertex
	// 	if < maxCoverage
	//	 	get the Direction
	//	 	if only 1 Direction
	//	 		if contig is not self
	//	 			get its length
	//	 			print link information
	//

	ReadAnnotation*a=&(m_readFetcher.getResult()->at(m_readAnnotationId));
	int rank=a->getRank();
	int sequenceId=a->getReadIndex();
	char strand=a->getStrand();
	int positionOnStrand=a->getPositionOnStrand();
	if(!m_hasPairRequested){
		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		buffer[0]=sequenceId;
		Message aMessage(buffer,1,MPI_UNSIGNED_LONG_LONG,rank,RAY_MPI_TAG_HAS_PAIRED_READ,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_hasPairRequested=true;
		m_hasPairReceived=false;
	}else if(!m_hasPairReceived
	&&m_virtualCommunicator->isMessageProcessed(m_workerId)){
		m_hasPair=m_virtualCommunicator->getMessageResponseElements(m_workerId)[0];
		m_hasPairReceived=true;
		m_pairRequested=false;
	}else if(!m_hasPairReceived){
		return;
	}else if(!m_hasPair){
		m_readAnnotationId++;
		m_hasPairRequested=false;
	}else if(!m_pairRequested){
		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		buffer[0]=sequenceId;
		Message aMessage(buffer,1,MPI_UNSIGNED_LONG_LONG,
		rank,RAY_MPI_TAG_GET_READ_MATE,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_pairRequested=true;
		m_pairReceived=false;
	}else if(!m_pairReceived
	&&m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<uint64_t> response=m_virtualCommunicator->getMessageResponseElements(m_workerId);
		m_readLength=response[0];
		m_pairedReadRank=response[1];
		m_pairedReadIndex=response[2];
		m_pairedReadLibrary=response[3];
		m_pairReceived=true;
		m_markersRequested=false;
	}else if(!m_pairReceived){
		return;
	}else if(!m_markersRequested){
		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		buffer[0]=m_pairedReadIndex;
		Message aMessage(buffer,1,MPI_UNSIGNED_LONG_LONG,
		m_pairedReadRank,RAY_MPI_TAG_GET_READ_MARKERS,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_markersRequested=true;
		m_markersReceived=false;
	}else if(!m_markersReceived
	&&m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<uint64_t> response=m_virtualCommunicator->getMessageResponseElements(m_workerId);
		int bufferPosition=0;
		m_pairedReadLength=response[bufferPosition++];
		m_pairedForwardMarker.unpack(&response,&bufferPosition);
		m_pairedReverseMarker.unpack(&response,&bufferPosition);
		m_pairedForwardOffset=response[bufferPosition++];
		m_pairedReverseOffset=response[bufferPosition++];
		m_markersReceived=true;
		m_forwardDirectionsRequested=false;
	}else if(!m_markersReceived){
		return;
/***
 *
 * Forward Directions
 *
 ***/
	}else if(!m_forwardDirectionsRequested){
		// skip unrelated marker
		if(m_pairedForwardOffset>m_pairedReadLength-m_parameters->getWordSize()){
			m_forwardDirectionsRequested=true;
			m_forwardDirectionsReceived=true;
			m_forwardDirectionLengthRequested=true;
			m_forwardDirectionLengthReceived=true;
		}
		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		int bufferPosition=0;
		m_pairedForwardMarker.pack(buffer,&bufferPosition);
		Message aMessage(buffer,bufferPosition,MPI_UNSIGNED_LONG_LONG,
		m_parameters->_vertexRank(&m_pairedForwardMarker),
		RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_forwardDirectionsRequested=true;
		m_forwardDirectionsReceived=false;
	}else if(!m_forwardDirectionsReceived
	&&m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<uint64_t> response=m_virtualCommunicator->getMessageResponseElements(m_workerId);
		m_pairedForwardMarkerCoverage=response[0];
		m_pairedForwardHasDirection=response[1];
		m_pairedForwardDirectionName=response[2];
		m_pairedForwardDirectionPosition=response[3];
		m_forwardDirectionsReceived=true;
		m_reverseDirectionsRequested=false;
		m_forwardDirectionLengthRequested=false;

		if(m_contigNames[m_contigId]==m_pairedForwardDirectionName
		||!(m_pairedForwardMarkerCoverage<m_parameters->getRepeatCoverage())
		|| !m_pairedForwardHasDirection){
			m_forwardDirectionLengthRequested=true;
			m_forwardDirectionLengthReceived=true;
		}
	}else if(!m_forwardDirectionsReceived){
		return;
	}else if(!m_forwardDirectionLengthRequested){
		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		int rankId=getRankFromPathUniqueId(m_pairedForwardDirectionName);
		buffer[0]=m_pairedForwardDirectionName;
		Message aMessage(buffer,1,MPI_UNSIGNED_LONG_LONG,
		rankId,
		RAY_MPI_TAG_GET_PATH_LENGTH,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_forwardDirectionLengthRequested=true;
		m_forwardDirectionLengthReceived=false;

	}else if(!m_forwardDirectionLengthReceived
	&&m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<uint64_t> response=m_virtualCommunicator->getMessageResponseElements(m_workerId);
		m_pairedForwardDirectionLength=response[0];
		m_forwardDirectionLengthReceived=true;

		int range=m_parameters->getLibraryAverageLength(m_pairedReadLibrary)+3*m_parameters->getLibraryStandardDeviation(m_pairedReadLibrary);

		if(m_pairedForwardDirectionLength<range
		||(int)m_contigs[m_contigId].size()<range
		|| 2*m_receivedCoverage<m_pairedForwardMarkerCoverage
		|| 2*m_pairedForwardMarkerCoverage<m_receivedCoverage ){
			return;
		}

		#ifdef PRINT_RAW_LINK
		cout<<endl;
		cout<<"AverageDistance: "<<m_parameters->getLibraryAverageLength(m_pairedReadLibrary)<<endl;
		cout<<"StandardDeviation: "<<m_parameters->getLibraryStandardDeviation(m_pairedReadLibrary)<<endl;
		cout<<"Path1: "<<m_contigNames[m_contigId]<<endl;
		cout<<" Length: "<<m_contigs[m_contigId].size()<<endl;
		cout<<" Position: "<<m_positionOnContig<<endl;
		cout<<" Coverage: "<<m_receivedCoverage<<endl;
		cout<<" PathStrand: F"<<endl;
		cout<<" ReadStrand: "<<strand<<endl;
		cout<<" ReadLength: "<<m_readLength<<endl;
		cout<<" PositionInRead: "<<positionOnStrand<<endl;
		cout<<"Path2: "<<m_pairedForwardDirectionName<<endl;
		cout<<" Length: "<<m_pairedForwardDirectionLength<<endl;
		cout<<" Position: "<<m_pairedForwardDirectionPosition<<endl;
		cout<<" Coverage: "<<m_pairedForwardMarkerCoverage<<endl;
		cout<<" PathStrand: F"<<endl;
		cout<<" ReadStrand: F"<<endl;
		cout<<" ReadLength: "<<m_pairedReadLength<<endl;
		cout<<" PositionInRead: "<<m_pairedForwardOffset<<endl;
		#endif

		bool path1IsLeft=false;
		bool path1IsRight=false;
		bool path2IsLeft=false;
		bool path2IsRight=false;
		if(m_positionOnContig<range)
			path1IsLeft=true;
		if(m_positionOnContig>(int)m_contigs[m_contigId].size()-range)
			path1IsRight=true;
		if(m_pairedForwardDirectionPosition<range)
			path2IsLeft=true;
		if(m_pairedForwardDirectionPosition>m_pairedForwardDirectionLength-range)
			path2IsRight=true;

		if((path1IsLeft&&path1IsRight)||(path2IsLeft&&path2IsRight))
			return;
/*
Case 6. (allowed)

                    ---->                              
                                                           ---->
------------------------>              ------------------------>
*/

		if(path1IsRight&&path2IsRight&&strand=='F'){
			int distanceIn1=m_contigs[m_contigId].size()-m_positionOnContig+positionOnStrand;
			int distanceIn2=m_pairedForwardDirectionLength-m_pairedForwardDirectionPosition+m_pairedForwardOffset;
			int distance=range-distanceIn1-distanceIn2;
			if(distance>0){
				m_scaffoldingSummary[m_contigNames[m_contigId]]['F'][m_pairedForwardDirectionName]['R'].push_back(distance);
				#ifdef PRINT_RAW_LINK
				cout<<"LINK06 "<<m_contigNames[m_contigId]<<",F,"<<m_pairedForwardDirectionName<<",R,"<<distance<<endl;
				#endif
			}
/*
Case 1. (allowed)

---->                              
                                       ---->
------------------------>              ------------------------>
*/
		}else if(path1IsLeft&&path2IsLeft&&strand=='F'){
			int distanceIn1=m_positionOnContig+m_readLength-positionOnStrand;
			int distanceIn2=m_pairedForwardDirectionPosition+m_pairedReadLength-m_pairedForwardOffset;
			int distance=range-distanceIn1-distanceIn2;
			if(distance>0){
				#ifdef PRINT_RAW_LINK
				cout<<"LINK01 "<<m_contigNames[m_contigId]<<",R,"<<m_pairedForwardDirectionName<<",F,"<<distance<<endl;
				#endif
				m_scaffoldingSummary[m_contigNames[m_contigId]]['R'][m_pairedForwardDirectionName]['F'].push_back(distance);
			}
/*
Case 10. (allowed)

<----
                                                           ---->
------------------------>              ------------------------>

                   ---->              <----
<-----------------------              <-------------------------
*/
		}else if(path1IsLeft&&path2IsRight&&strand=='R'){
			int distanceIn1=m_positionOnContig+positionOnStrand;
			int distanceIn2=m_pairedForwardDirectionLength-m_pairedForwardDirectionPosition+m_pairedForwardOffset;
			int distance=range-distanceIn1-distanceIn2;
			if(distance>0){
				#ifdef PRINT_RAW_LINK
				cout<<"LINK10 "<<m_contigNames[m_contigId]<<",R,"<<m_pairedForwardDirectionName<<",R,"<<distance<<endl;
				#endif
				m_scaffoldingSummary[m_contigNames[m_contigId]]['R'][m_pairedForwardDirectionName]['R'].push_back(distance);
			}

/*
Case 13. (allowed)

                    <----
                                       ---->
------------------------>              ------------------------>
*/
		}else if(path1IsRight&&path2IsLeft&&strand=='R'){
			int distanceIn1=m_contigs[m_contigId].size()-m_positionOnContig-positionOnStrand+m_readLength;
			int distanceIn2=m_pairedForwardDirectionPosition+m_pairedReadLength-m_pairedForwardOffset;
			int distance=range-distanceIn1-distanceIn2;
			if(distance>0){
				#ifdef PRINT_RAW_LINK
				cout<<"LINK13 "<<m_contigNames[m_contigId]<<",F,"<<m_pairedForwardDirectionName<<",F,"<<distance<<endl;
				#endif
				m_scaffoldingSummary[m_contigNames[m_contigId]]['F'][m_pairedForwardDirectionName]['F'].push_back(distance);
			}
		}

	}else if(!m_forwardDirectionLengthReceived){
		return;

/***
 *
 * Reverse Directions
 *
 ***/
	}else if(!m_reverseDirectionsRequested){
		if(m_pairedReverseOffset>m_pairedReadLength-m_parameters->getWordSize()){
			m_reverseDirectionsRequested=true;
			m_reverseDirectionsReceived=true;
			m_reverseDirectionLengthRequested=true;
			m_reverseDirectionLengthReceived=true;
		}

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		int bufferPosition=0;
		m_pairedReverseMarker.pack(buffer,&bufferPosition);
		Message aMessage(buffer,bufferPosition,MPI_UNSIGNED_LONG_LONG,
		m_parameters->_vertexRank(&m_pairedReverseMarker),
		RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_reverseDirectionsRequested=true;
		m_reverseDirectionsReceived=false;
	}else if(!m_reverseDirectionsReceived
	&&m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<uint64_t> response=m_virtualCommunicator->getMessageResponseElements(m_workerId);
		m_pairedReverseMarkerCoverage=response[0];
		m_pairedReverseHasDirection=response[1];
		m_pairedReverseDirectionName=response[2];
		m_pairedReverseDirectionPosition=response[3];
		m_reverseDirectionsReceived=true;
		m_reverseDirectionLengthRequested=false;

		if(m_contigNames[m_contigId]==m_pairedReverseDirectionName
		||!(m_pairedReverseMarkerCoverage<m_parameters->getRepeatCoverage())
		|| !m_pairedReverseHasDirection){
			m_reverseDirectionLengthRequested=true;
			m_reverseDirectionLengthReceived=true;
		}
	}else if(!m_reverseDirectionsReceived){
		return;
	}else if(!m_reverseDirectionLengthRequested){
		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		int rankId=getRankFromPathUniqueId(m_pairedReverseDirectionName);
		buffer[0]=m_pairedReverseDirectionName;
		Message aMessage(buffer,1,MPI_UNSIGNED_LONG_LONG,
		rankId,RAY_MPI_TAG_GET_PATH_LENGTH,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_reverseDirectionLengthRequested=true;
		m_reverseDirectionLengthReceived=false;
	}else if(!m_reverseDirectionLengthReceived
	&&m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<uint64_t> response=m_virtualCommunicator->getMessageResponseElements(m_workerId);
		m_pairedReverseDirectionLength=response[0];
		m_reverseDirectionLengthReceived=true;
		
		int range=m_parameters->getLibraryAverageLength(m_pairedReadLibrary)+3*m_parameters->getLibraryStandardDeviation(m_pairedReadLibrary);

		if(m_pairedReverseDirectionLength<range
		||(int)m_contigs[m_contigId].size()<range
		|| 2*m_receivedCoverage<m_pairedReverseMarkerCoverage
		|| 2*m_pairedReverseMarkerCoverage<m_receivedCoverage){
			return;
		}
	
		#ifdef PRINT_RAW_LINK
		cout<<endl;
		cout<<"AverageDistance: "<<m_parameters->getLibraryAverageLength(m_pairedReadLibrary)<<endl;
		cout<<"StandardDeviation: "<<m_parameters->getLibraryStandardDeviation(m_pairedReadLibrary)<<endl;
		cout<<"Path1: "<<m_contigNames[m_contigId]<<endl;
		cout<<" Length: "<<m_contigs[m_contigId].size()<<endl;
		cout<<" Position: "<<m_positionOnContig<<endl;
		cout<<" Coverage: "<<m_receivedCoverage<<endl;
		cout<<" PathStrand: F"<<endl;
		cout<<" ReadStrand: "<<strand<<endl;
		cout<<" ReadLength: "<<m_readLength<<endl;
		cout<<" PositionInRead: "<<positionOnStrand<<endl;
		cout<<"Path2: "<<m_pairedReverseDirectionName<<endl;
		cout<<" Length: "<<m_pairedReverseDirectionLength<<endl;
		cout<<" Position: "<<m_pairedReverseDirectionPosition<<endl;
		cout<<" Coverage: "<<m_pairedReverseMarkerCoverage<<endl;
		cout<<" PathStrand: F"<<endl;
		cout<<" ReadStrand: R"<<endl;
		cout<<" ReadLength: "<<m_pairedReadLength<<endl;
		cout<<" PositionInRead: "<<m_pairedReverseOffset<<endl;
		#endif

		bool path1IsLeft=false;
		bool path1IsRight=false;
		bool path2IsLeft=false;
		bool path2IsRight=false;
		if(m_positionOnContig<range)
			path1IsLeft=true;
		if(m_positionOnContig>(int)m_contigs[m_contigId].size()-range)
			path1IsRight=true;
		if(m_pairedReverseDirectionPosition<range)
			path2IsLeft=true;
		if(m_pairedReverseDirectionPosition>m_pairedReverseDirectionLength-range)
			path2IsRight=true;

		if((path1IsLeft&&path1IsRight)||(path2IsLeft&&path2IsRight))
			return;

/*
Case 4. (allowed)

---->                              
                                                           <----
------------------------>              ------------------------>
*/

		if(path1IsLeft&&path2IsRight&&strand=='F'){
			int distanceIn1=m_positionOnContig+m_readLength-positionOnStrand;
			int distanceIn2=m_pairedReverseDirectionLength-m_pairedReverseDirectionPosition-m_pairedReverseOffset+m_pairedReadLength;
			int distance=range-distanceIn1-distanceIn2;
			if(distance>0){
				#ifdef PRINT_RAW_LINK
				cout<<"LINK04 "<<m_contigNames[m_contigId]<<",R,"<<m_pairedReverseDirectionName<<",R,"<<distance<<endl;
				#endif
				m_scaffoldingSummary[m_contigNames[m_contigId]]['R'][m_pairedReverseDirectionName]['R'].push_back(distance);
			}
		

/*
Case 7. (allowed)

                    ---->                              
                                       <----
------------------------>              ------------------------>
*/
		}else if(path1IsRight&&path2IsLeft&&strand=='F'){
			int distanceIn1=m_contigs[m_contigId].size()-m_positionOnContig+positionOnStrand;
			int distanceIn2=m_pairedReverseDirectionPosition+m_pairedReverseOffset;
			int distance=range-distanceIn1-distanceIn2;
			if(distance>0){
				#ifdef PRINT_RAW_LINK
				cout<<"LINK07 "<<m_contigNames[m_contigId]<<",F,"<<m_pairedReverseDirectionName<<",F,"<<distance<<endl;
				#endif
				m_scaffoldingSummary[m_contigNames[m_contigId]]['F'][m_pairedReverseDirectionName]['F'].push_back(distance);
			}
	

/*
Case 11. (allowed)

<----
                                       <----
------------------------>              ------------------------>
*/
		}else if(path1IsLeft&&path2IsLeft&&strand=='R'){
			int distanceIn1=m_positionOnContig+positionOnStrand;
			int distanceIn2=m_pairedReverseDirectionPosition+m_pairedReverseOffset;
			int distance=range-distanceIn1-distanceIn2;
			if(distance>0){
				#ifdef PRINT_RAW_LINK
				cout<<"LINK11 "<<m_contigNames[m_contigId]<<",R,"<<m_pairedReverseDirectionName<<",F,"<<distance<<endl;
				#endif
				m_scaffoldingSummary[m_contigNames[m_contigId]]['R'][m_pairedReverseDirectionName]['F'].push_back(distance);
			}

/*
Case 16. (allowed)

                    <----
                                                           <----
------------------------>              ------------------------>
*/
		}else if(path1IsRight&&path2IsRight&&strand=='R'){
			int distanceIn1=m_contigs[m_contigId].size()-m_positionOnContig-positionOnStrand+m_readLength;
			int distanceIn2=m_pairedReverseDirectionLength-m_pairedReverseDirectionPosition-m_pairedReverseOffset+m_pairedReadLength;
			int distance=range-distanceIn1-distanceIn2;
			if(distance>0){
				#ifdef PRINT_RAW_LINK
				cout<<"LINK16 "<<m_contigNames[m_contigId]<<",F,"<<m_pairedReverseDirectionName<<",R,"<<distance<<endl;
				#endif
				m_scaffoldingSummary[m_contigNames[m_contigId]]['F'][m_pairedReverseDirectionName]['R'].push_back(distance);
			}
		}
	}else if(!m_reverseDirectionLengthReceived){
		return;

	}else if(m_reverseDirectionLengthReceived){
		m_readAnnotationId++;
		m_hasPairRequested=false;
	}
}

void Scaffolder::getContigSequence(uint64_t id){
	if(!m_hasContigSequence_Initialised){
		m_hasContigSequence_Initialised=true;
		m_rankIdForContig=getRankFromPathUniqueId(id);
		m_theLength=m_contigLengths[id];
		m_position=0;
		m_contigPath.clear();
		m_requestedContigChunk=false;
	}
	
	if(m_position<m_theLength){
		if(!m_requestedContigChunk){
			m_requestedContigChunk=true;
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
			message[0]=id;
			message[1]=m_position;
			Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,
				m_rankIdForContig,RAY_MPI_TAG_GET_CONTIG_CHUNK,m_parameters->getRank());
			m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
			vector<uint64_t> data=m_virtualCommunicator->getMessageResponseElements(m_workerId);
			int pos=0;
			while(pos<(int)data.size()){
				Kmer a;
				a.unpack(&data,&pos);
				m_contigPath.push_back(a);
				m_position++;
			}
			m_requestedContigChunk=false;
		}
	}else{
		m_contigSequence=convertToString(&m_contigPath,m_parameters->getWordSize());
		m_hasContigSequence=true;
	}
}

void Scaffolder::writeScaffolds(){
	if(!m_initialised){
		m_initialised=true;
		m_scaffoldId=0;
		m_contigId=0;
		/* actually it is a position on the scaffold */
		m_positionOnScaffold=0;
		m_hasContigSequence=false;
		m_hasContigSequence_Initialised=false;
		string file=m_parameters->getScaffoldFile();
		m_fp=fopen(file.c_str(),"w");
	}

	m_virtualCommunicator->forceFlush();
	m_virtualCommunicator->processInbox(&m_activeWorkers);
	m_activeWorkers.clear();

	if(m_scaffoldId<(int)m_scaffoldContigs.size()){
		if(m_contigId<(int)m_scaffoldContigs[m_scaffoldId].size()){
			uint64_t contigNumber=m_scaffoldContigs[m_scaffoldId][m_contigId];
			if(!m_hasContigSequence){
				getContigSequence(contigNumber);
			}else{ /* at this point, m_contigSequence is filled. */
				if(m_contigId==0){
					fprintf(m_fp,">scaffold-%i\n",m_scaffoldId);
					m_positionOnScaffold=0;
				}

				int contigPosition=0;
				char strand=m_scaffoldStrands[m_scaffoldId][m_contigId];
				if(strand=='R'){
					m_contigSequence=reverseComplement(&m_contigSequence);
				}

				int length=m_contigSequence.length();
				int columns=m_parameters->getColumns();
				ostringstream outputBuffer;
				while(contigPosition<length){
					char nucleotide=m_contigSequence[contigPosition];
					outputBuffer<<nucleotide;
					contigPosition++;
					m_positionOnScaffold++;
					if(m_positionOnScaffold%columns==0){
						outputBuffer<<"\n";
					}
				}
				
				fprintf(m_fp,"%s",outputBuffer.str().c_str());

				if(m_contigId<(int)m_scaffoldContigs[m_scaffoldId].size()-1){
					int gapSize=m_scaffoldGaps[m_scaffoldId][m_contigId];
					int i=0;
					int columns=m_parameters->getColumns();
					ostringstream outputBuffer2;
					while(i<gapSize){
						outputBuffer2<<"N";
						i++;
						m_positionOnScaffold++;
						if(m_positionOnScaffold%columns==0){
							outputBuffer2<<"\n";
						}
					}
					fprintf(m_fp,"%s",outputBuffer2.str().c_str());
				}
				m_contigId++;
				m_hasContigSequence=false;
				m_hasContigSequence_Initialised=false;
			}
		}else{
			fprintf(m_fp,"\n");
			m_scaffoldId++;
			m_contigId=0;
			m_positionOnScaffold=0;
			m_hasContigSequence=false;
			m_hasContigSequence_Initialised=false;
		}
	}else{
		fclose(m_fp);
		m_parameters->setMasterMode(RAY_MASTER_MODE_KILL_RANKS);
	}
}

/*

Case 1. (allowed)

---->                              
                                       ---->
------------------------>              ------------------------>


Case 2. (disallowed)

---->                              
                                                           ---->
------------------------>              ------------------------>

Case 3. (disallowed)

---->                              
                                       <----
------------------------>              ------------------------>

Case 4. (allowed)

---->                              
                                                           <----
------------------------>              ------------------------>

Case 5. (disallowed)

                    ---->                              
                                       ---->
------------------------>              ------------------------>


Case 6. (allowed)

                    ---->                              
                                                           ---->
------------------------>              ------------------------>

Case 7. (allowed)

                    ---->                              
                                       <----
------------------------>              ------------------------>

Case 8. (disallowed)

                    ---->                              
                                                           <----
------------------------>              ------------------------>


Case 9. (disallowed)

<----
                                       ---->
------------------------>              ------------------------>


Case 10. (allowed)

<----
                                                           ---->
------------------------>              ------------------------>

Case 11. (allowed)

<----
                                       <----
------------------------>              ------------------------>

Case 12. (disallowed)

<----
                                                           <----
------------------------>              ------------------------>

Case 13. (allowed)

                    <----
                                       ---->
------------------------>              ------------------------>


Case 14. (disallowed)

                    <----
                                                           ---->
------------------------>              ------------------------>

Case 15. (disallowed)

                    <----
                                       <----
------------------------>              ------------------------>

Case 16. (allowed)

                    <----
                                                           <----
------------------------>              ------------------------>

*/
