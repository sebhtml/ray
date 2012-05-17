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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <plugin_SequencesLoader/ArrayOfReads.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
using namespace std;

void ArrayOfReads::constructor(MyAllocator*allocator){
	#ifdef ASSERT
	assert(allocator!=NULL);
	#endif
	m_allocator=allocator;
	m_CHUNK_SIZE=100000;
	m_maxNumberOfChunks=100000;

	/* increased the maximum number of chunks... */
	m_chunks=(Read**)m_allocator->allocate(m_maxNumberOfChunks*sizeof(Read*));
	for(int i=0;i<m_maxNumberOfChunks;i++)
		m_chunks[i]=NULL;

	#ifdef ASSERT
	assert(m_chunks!=NULL);
	#endif

	m_elements=0;
	m_numberOfChunks=0;
	m_maxSize=0;
}

void ArrayOfReads::push_back(Read*a){
	#ifdef ASSERT
	assert(a!=NULL);
	#endif
	if(m_elements==m_maxSize){
		#ifdef ASSERT
		assert(m_numberOfChunks!=m_maxNumberOfChunks);
		#endif

		m_maxSize+=m_CHUNK_SIZE;
		m_numberOfChunks++;
		#ifdef ASSERT
		assert(m_numberOfChunks!=0);
		#endif

		#ifdef ASSERT
		assert(m_chunks!=NULL);
		#endif

		m_chunks[m_numberOfChunks-1]=(Read*)m_allocator->allocate(m_CHUNK_SIZE*sizeof(Read));

		#ifdef ASSERT
		assert(m_chunks[m_numberOfChunks-1]!=NULL);
		assert(m_elements<m_maxSize);
		assert(m_numberOfChunks!=0);
		assert(m_maxSize!=0);
		#endif
	}

	#ifdef ASSERT
	assert(m_elements!=m_maxSize);
	assert(m_maxSize!=0);
	if(m_numberOfChunks==0){
		cout<<"ChunkSize="<<m_CHUNK_SIZE<<" ChunksPointer="<<m_chunks<<" NumberOfChunks="<<m_numberOfChunks<<" NumberOfElements="<<m_elements<<" MaxNumberOfElements="<<m_maxSize<<endl;
	}
	assert(m_numberOfChunks!=0);
	#endif

	m_elements++;
	Read*b=at(m_elements-1);
	*b=*a;

	#ifdef ASSERT
	assert(m_elements<=m_maxSize);
	assert(m_elements!=0);
	assert(m_maxSize!=0);
	assert(m_chunks!=0);
	assert(m_numberOfChunks!=0);
	#endif
}

LargeCount ArrayOfReads::size(){
	return m_elements;
}

Read*ArrayOfReads::at(LargeIndex i){
	#ifdef ASSERT
	assert(m_maxSize!=0);
	assert(m_numberOfChunks!=0);
	assert(m_elements!=0);
	assert(m_chunks!=NULL);
	assert(i<m_elements);
	#endif

	int chunkNumber=i/m_CHUNK_SIZE;

	#ifdef ASSERT
	if(chunkNumber>=m_numberOfChunks){
		cout<<"ElementIdentifier="<<i<<" ChunkIdentifier="<<chunkNumber<<" NumberOfChunks="<<m_numberOfChunks<<" NumberOfElements="<<m_elements<<" MaxNumberOfElements="<<m_maxSize<<endl;
	}
	assert(chunkNumber<m_numberOfChunks);
	#endif

	int positionInSaidChunk=i%m_CHUNK_SIZE;
	
	#ifdef ASSERT
	assert(positionInSaidChunk<m_CHUNK_SIZE);
	#endif

	Read*theRead=m_chunks[chunkNumber]+positionInSaidChunk;
	return theRead;
}

Read*ArrayOfReads::operator[](LargeIndex i){
	return at(i);
}

void ArrayOfReads::clear(){
	if(m_chunks!=NULL){
		m_chunks=NULL;
		m_elements=0;
		m_numberOfChunks=0;
		m_maxSize=0;
	}

	#ifdef ASSERT
	assert(m_chunks==NULL);
	assert(m_elements==0);
	assert(m_numberOfChunks==0);
	assert(m_maxSize==0);
	#endif
}

void ArrayOfReads::reset(){
	m_elements=0;
	m_numberOfChunks=0;
	m_maxSize=0;
	m_chunks=(Read**)m_allocator->allocate(m_maxNumberOfChunks*sizeof(Read*));
}
