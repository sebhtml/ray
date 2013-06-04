/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#include "FastqLoader.h"

#include <code/Mock/constants.h>

#include <fstream>
#include <stdlib.h>
using namespace std;

#define SIZE_4MiB 4194304

#define CONFIG_FASTQ_BUFFER_SIZE SIZE_4MiB

FastqLoader::FastqLoader() {
	addExtension(".fastq");
	addExtension(".fq");

	clearPointers();
}

int FastqLoader::open(string file){
	return openWithPeriod(file,4);
}

/**
 * This is a drop-in replacement for fgets with buffering features.
 *
 * \see http://www.cplusplus.com/reference/cstdio/fgets/
 * \see http://www.cplusplus.com/reference/cstdio/feof/
 * \see http://www.cplusplus.com/reference/cstdio/fread/
 *
 * TODO This method will not work properly if numberOfBytes is > CONFIG_FASTQ_BUFFER_SIZE
 * TODO Also, it will not work if lines from the content stream are longer than CONFIG_FASTQ_BUFFER_SIZE.
 *
 * \author Sébastien Boisvert
 * \see https://github.com/sebhtml/ray/issues/137
 */
char * FastqLoader::streamContent(char * content, int numberOfBytes, FILE * endpoint){

	// return fgets(content, numberOfBytes, endpoint);

	char newLine = '\n';
	int firstNewLinePosition = 0;
	char * result = NULL;
	char terminatingCharacter = '\0';

#ifdef CONFIG_FASTQ_DEBUG_MESSAGE
	cout << "[streamContent] m_assetCacheLength is " << m_assetCacheLength << endl;
#endif

	// find the first \n
	while((m_assetCacheOffset + firstNewLinePosition) < m_assetCacheLength
			&& m_assetCacheContent[m_assetCacheOffset + firstNewLinePosition] != newLine)
		firstNewLinePosition ++;

	// try to stream more content if no \n was found.
	if( (m_assetCacheContent[m_assetCacheOffset + firstNewLinePosition] != newLine
		|| m_assetCacheLength == 0) && !feof(endpoint)) {

#ifdef CONFIG_FASTQ_DEBUG_MESSAGE
		cout << "[streamContent] trying to stream data." << endl;
#endif

		// move data on the left.
		int source = m_assetCacheOffset;
		int destination = 0;
		while(source < m_assetCacheLength) {
			m_assetCacheContent[destination] = m_assetCacheContent[source];
			source++;
			destination++;
		}

		m_assetCacheLength = destination;
		m_assetCacheOffset = 0;

#ifdef CONFIG_FASTQ_DEBUG_MESSAGE
		cout << "[streamContent] bytes after packing: " << m_assetCacheLength << endl;
#endif

		int bytes = m_assetCacheMaximumLength;
		bytes -= m_assetCacheLength;

		// we don't care if we overwrite the \0, in fact, we want this event to happen for sure.
		if(m_assetCacheLength > 0)
			bytes ++;

		#ifdef CONFIG_ASSERT
		assert(bytes <= m_assetCacheMaximumLength);
		assert(m_assetCacheMaximumLength >= 1);
		#endif

#ifdef CONFIG_FASTQ_DEBUG_MESSAGE
		cout << "[streamContent] trying to read " << bytes << " bytes into ";
		cout << hex << m_assetCacheContent << dec << " + " << m_assetCacheLength << endl;
#endif

		// stream content and append that to the asset cache content.
		int elements = fread(m_assetCacheContent + m_assetCacheLength, sizeof(char), bytes, endpoint);

#ifdef CONFIG_FASTQ_DEBUG_MESSAGE
		cout << "[streamContent] " << elements << " bytes streamed." << endl;
#endif

		m_assetCacheLength += elements;

		// recursive call to go outside this scope.
		// actually we don't need the recursion here...
		// streamContent(content, numberOfBytes, endpoint);
	}

#ifdef CONFIG_FASTQ_DEBUG_MESSAGE
	cout << "[streamContent] " << m_assetCacheLength << " bytes available in the asset cache" << endl;
#endif

	// copy data into the content from the client.
	//
	// This code manages 3 cases:
	//
	// case 001: the asset cache contains 0 bytes
	// case 002: the asset cache contains x bytes, but no \n
	// case 003: the asset cache contains x bytes, with a \n somewhere in there.

	int destination = 0;

	while(m_assetCacheOffset < m_assetCacheLength) { // this condition is necessary to avoid streaming 0 bytes.

		char data = m_assetCacheContent[m_assetCacheOffset];
		content[destination] = data;
		result = content;
		destination ++;
		m_assetCacheOffset ++;

		if(destination + 1 == numberOfBytes // the client buffer is full, we just need to add \0
			|| m_assetCacheOffset == m_assetCacheLength // we don't have any more data
			|| data == newLine) { // we stop at the new line

#ifdef CONFIG_FASTQ_DEBUG_MESSAGE
			cout << "[streamContent] final destination is " << destination << endl;
#endif

			content[destination] = terminatingCharacter;
			break;
		}
	}

	return result;
}

int FastqLoader::openWithPeriod(string file,int period){
	m_f=fopen(file.c_str(),"r");

	m_assetCacheMaximumLength = CONFIG_FASTQ_BUFFER_SIZE;
	m_assetCacheLength = 0;
	m_assetCacheContent = (char*) malloc(m_assetCacheMaximumLength * sizeof(char));
	m_assetCacheOffset = 0;

	m_size=0;
	m_loaded=0;
	int rotatingVariable=0;
	char buffer[RAY_MAXIMUM_READ_LENGTH];
	while(NULL!= streamContent(buffer,RAY_MAXIMUM_READ_LENGTH,m_f)){
		if(rotatingVariable==1){
			m_size++;
		}
		rotatingVariable++;
		if(rotatingVariable==period){
			rotatingVariable=0;
		}
	}

	fclose(m_f);
	m_f=fopen(file.c_str(),"r");
	return EXIT_SUCCESS;
}

void FastqLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
	loadWithPeriod(maxToLoad,reads,seqMyAllocator,4);
}

void FastqLoader::loadWithPeriod(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator,int period){
	char buffer[RAY_MAXIMUM_READ_LENGTH];
	int rotatingVariable=0;
	int loadedSequences=0;

	while(loadedSequences<maxToLoad && NULL!=streamContent(buffer,RAY_MAXIMUM_READ_LENGTH,m_f)){
		if(rotatingVariable==1){
			Read t;
			t.constructor(buffer,seqMyAllocator,true);
			reads->push_back(&t);
		}
		rotatingVariable++;

		// a period is reached for each read.
		if(rotatingVariable==period){
			rotatingVariable=0;
			loadedSequences++;
			m_loaded++;
		}
	}

	if(m_loaded==m_size){
		close();
	}
}

int FastqLoader::getSize(){
	return m_size;
}

void FastqLoader::close(){
	if(m_f != NULL) {
		fclose(m_f);
	}

	if(m_assetCacheContent != NULL) {
		free(m_assetCacheContent);
	}

	clearPointers();
}

void FastqLoader::clearPointers() {

	m_f = NULL;
	m_assetCacheContent = NULL;
	m_assetCacheLength = 0;
	m_assetCacheMaximumLength = 0;
}
