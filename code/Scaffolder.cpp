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

#include <Scaffolder.h>
#include <Message.h>
#include <vector>
using namespace std;

void Scaffolder::constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,Parameters*parameters,
	int*slaveMode){
	m_outbox=outbox;
	m_inbox=inbox;
	m_outboxAllocator=outboxAllocator;
	m_parameters=parameters;
	m_slave_mode=slaveMode;
	m_initialised=false;
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

	if(m_contigId<(int)m_contigs.size()){
		if(m_positionOnContig<(int)m_contigs[m_contigId].size()){
			VERTEX_TYPE vertex=m_contigs[m_contigId][m_positionOnContig];
			VERTEX_TYPE reverseComplement=m_parameters->_complementVertex(vertex);
			if(!m_forwardDone){
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
					uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));

					Message aMessage(buffer,1,MPI_UNSIGNED_LONG_LONG,
					m_parameters->_vertexRank(vertex),RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_parameters->getRank());
					m_outbox->push_back(aMessage);
					m_coverageRequested=true;
					m_coverageReceived=false;
				}else if(m_inbox->size()==1
					&&m_inbox->at(0)->getTag()==RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY){
					m_receivedCoverage=((uint64_t*)m_inbox->at(0)->getBuffer())[0];
					m_coverageReceived=true;
				}else if(m_coverageReceived){
					cout<<"Coverage= "<<m_receivedCoverage<<endl;
					m_forwardDone=true;
					m_reverseDone=false;
				}
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
			}
		}else{
			m_contigId++;
			m_positionOnContig=0;
			m_forwardDone=false;
		}
	}else{
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_I_FINISHED_SCAFFOLDING,
			m_parameters->getRank());
		m_outbox->push_back(aMessage);
		(*m_slave_mode)=RAY_SLAVE_MODE_DO_NOTHING;
	}
}

void Scaffolder::addContig(uint64_t name,vector<uint64_t>*vertices){
	m_contigNames.push_back(name);
	m_contigs.push_back(*vertices);
}
