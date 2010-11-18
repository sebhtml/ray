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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/


#include<FusionData.h>
#include<Message.h>

void FusionData::distribute(SeedingData*m_seedingData,ExtensionData*m_ed,int getRank,RingAllocator*m_outboxAllocator,StaticVector*m_outbox,int getSize,int*m_mode){
	if(!isReady()){
		return;
	}
	if(m_seedingData->m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){
		m_buffers.flush(3,TAG_SAVE_WAVE_PROGRESSION,m_outboxAllocator,m_outbox,getRank,true);
		cout<<"Rank "<<getRank<<" distributes its fusions. "<<m_ed->m_EXTENSION_contigs.size()<<"/"<<m_ed->m_EXTENSION_contigs.size()<<" (DONE)"<<endl;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_DISTRIBUTE_FUSIONS_FINISHED,getRank);
		m_outbox->push_back(aMessage);
		(*m_mode)=MODE_DO_NOTHING;
		return;
	}

	if(m_ed->m_EXTENSION_currentPosition==0){
		if(m_seedingData->m_SEEDING_i%10==0){
			cout<<"Rank "<<getRank<<" distributes its fusions. "<<m_seedingData->m_SEEDING_i+1<<"/"<<m_ed->m_EXTENSION_contigs.size()<<endl;

		}
	}

	VERTEX_TYPE vertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
	int destination=vertexRank(vertex,getSize);
	m_buffers.addAt(destination,vertex);
	m_buffers.addAt(destination,m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i]);
	m_buffers.addAt(destination,m_ed->m_EXTENSION_currentPosition);

	bool flushed=m_buffers.flush(3,TAG_SAVE_WAVE_PROGRESSION,m_outboxAllocator,m_outbox,getRank,false);
	if(flushed){
		m_ready=false;
	}
	m_ed->m_EXTENSION_currentPosition++;

	// the next one
	if(m_ed->m_EXTENSION_currentPosition==(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
		m_seedingData->m_SEEDING_i++;
		m_ed->m_EXTENSION_currentPosition=0;
	}
}

void FusionData::constructor(int size,int max){
	m_buffers.constructor(size,max);
}

FusionData::FusionData(){
	setReadiness();
}

void FusionData::setReadiness(){
	m_ready=true;
}

bool FusionData::isReady(){
	return m_ready;
}
