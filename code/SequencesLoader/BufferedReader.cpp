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

#include <iostream>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

	//cout << "[DEBUG] readLine" << endl;

	//return fgets(content, numberOfBytes, endpoint);

	return readLineFromBuffer(content, numberOfBytes, endpoint, true);
}

/**
 *
 */
char * BufferedReader::readLineFromBuffer(char * content, int numberOfBytes, FILE * endpoint, bool retry) {

	if(copyWithNewLine(content))
		return content;

	if(retry) {
		fillBuffer(endpoint);
		return readLineFromBuffer(content, numberOfBytes, endpoint, false);
	}

	if(copyWithMaximumNumberOfBytes(content, numberOfBytes))
		return content;

	return NULL;
}

void BufferedReader::fillBuffer(FILE * source) {

	//cout << "[DEBUG] fillBuffer " << endl;
	moveBuffer();

	char * buffer = getBuffer();
	int bufferSize = getBufferSize();
	int maximumBufferSize = getMaximumBufferSize();
	int available = maximumBufferSize - bufferSize;

	char * destination = buffer + bufferSize;

	int elements = fread(destination, sizeof(char), available, source);

	m_assetCacheLength += elements;
}

int BufferedReader::getMaximumBufferSize() {
	return m_assetCacheMaximumLength;
}

void BufferedReader::moveBuffer() {
	int source = m_assetCacheOffset;
	int destination = 0;

	while(source < m_assetCacheLength) {
		m_assetCacheContent[destination] = m_assetCacheContent[source];
		destination++;
		source++;
	}

	m_assetCacheLength -= m_assetCacheOffset;
	m_assetCacheOffset = 0;
}

char * BufferedReader::getBuffer() {
	return m_assetCacheContent + m_assetCacheOffset;
}

int BufferedReader::getBufferSize() {
	return m_assetCacheLength - m_assetCacheOffset;
}

int BufferedReader::findNewLine(char * sequence, int length) {

	int i = 0;

	while(i < length) {
		if(sequence[i] == '\n')
			return i;
		i++;
	}

	return -1;
}

bool BufferedReader::copyWithNewLine(char * content) {

	char * buffer = getBuffer();
	int bufferSize = getBufferSize();

	int newLinePosition = findNewLine(buffer, bufferSize);

	if(newLinePosition < 0)
		return false;

	int count = newLinePosition + 1;

	consumeContent(buffer, content, count);

	return true;
}

bool BufferedReader::copyWithMaximumNumberOfBytes(char * content, int numberOfBytes) {

	int availableBytes = getBufferSize();

	if(availableBytes == 0)
		return false;

	char * buffer = getBuffer();

	int count = numberOfBytes;

	if(availableBytes < count)
		count = availableBytes;

	consumeContent(buffer, content, count);

	return true;

}

void BufferedReader::consumeContent(char * buffer, char * content, int count) {
	memcpy(content, buffer, count);
	content[count] = '\0';

	m_assetCacheOffset += count;
}

void BufferedReader::destroy() {
	if(m_assetCacheContent != NULL) {
		free(m_assetCacheContent);
	}
	m_assetCacheContent = NULL;
	m_assetCacheLength = 0;
	m_assetCacheMaximumLength = 0;

}
