/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
 *  Copyright (C) 2013 Sébastien Boisvert
 *
 *  http://DeNovoAssembler.SourceForge.Net/
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You have received a copy of the GNU General Public License
 *  along with this program (gpl-3.0.txt).
 *  see <http://www.gnu.org/licenses/>
 */

#include "BufferedReader.h"

#include <code/Mock/constants.h>

#include <stdlib.h>
#include <stdio.h>

#ifdef CONFIG_ASSERT
#include <assert.h>
#endif

//#define CONFIG_FASTQ_DEBUG_MESSAGE
#define CONFIG_FASTQ_BUFFER_SIZE SIZE_4M

void BufferedReader::initialize() {
	m_assetCacheMaximumLength = CONFIG_FASTQ_BUFFER_SIZE;
	m_assetCacheContent = (char*) malloc(m_assetCacheMaximumLength * sizeof(char));

	reset();
}

void BufferedReader::reset() {
	m_assetCacheLength = 0;
	m_assetCacheOffset = 0;
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
 *
 *
 * m_assetCacheContent -> the content
 * m_assetCacheLength  -> the size of the content
 * m_assetCacheOffset  -> the current offset in the content
 * m_assetCacheMaximumLength -> the maximum size
 */
char * BufferedReader::readLine(char * content, int numberOfBytes, FILE * endpoint){

	return fgets(content, numberOfBytes, endpoint);

	char newLine = '\n';
	int firstNewLinePosition = 0;
	char * result = NULL;
	char terminatingCharacter = '\0';

#ifdef CONFIG_FASTQ_DEBUG_MESSAGE
	cout << "[streamContent] call, m_assetCacheLength is " << m_assetCacheLength << endl;
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

		/*
		cout << "[DEBUG] fread m_assetCacheContent= " << hex << (void*)m_assetCacheContent;
		cout << dec << " m_assetCacheLength= " << m_assetCacheLength << " bytes= " << bytes;
		cout << " endpoint= " << hex << endpoint << dec << endl;
		*/

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
	cout << "[streamContent] " << m_assetCacheLength << " bytes available in the asset cache";
	cout << " m_assetCacheOffset= " << m_assetCacheOffset << endl;
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
			cout << "[streamContent] resulting data size is " << destination << endl;
#endif

			content[destination] = terminatingCharacter;
			break;
		}
	}

	return result;
}

void BufferedReader::destroy() {
	if(m_assetCacheContent != NULL) {
		free(m_assetCacheContent);
	}
	m_assetCacheContent = NULL;
	m_assetCacheLength = 0;
	m_assetCacheMaximumLength = 0;

}
