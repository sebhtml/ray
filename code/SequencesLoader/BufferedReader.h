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



#ifndef BufferedReaderHeader
#define BufferedReaderHeader

#include <stdio.h>

/**
 * This is a buffered reader for files.
 *
 * readLine takes the same arguments than fgets
 *
 * \author Sébastien Boisvert
 */
class BufferedReader {

/**
 * This is the buffer for storing asset cache streamed from the endpoint.
 */
	char * m_assetCacheContent;
	int m_assetCacheLength;
	int m_assetCacheMaximumLength;
	int m_assetCacheOffset;

	char * readLineFromBuffer(char * content, int numberOfBytes, FILE * endpoint, bool retry);
	void fillBuffer(FILE * source);
	int getMaximumBufferSize();
	void moveBuffer();
	char * getBuffer();
	int getBufferSize();
	int findNewLine(char * sequence, int length);
	bool copyWithNewLine(char * content);
	bool copyWithMaximumNumberOfBytes(char * content, int numberOfBytes);

	void consumeContent(char * buffer, char * content, int count);

public:

	void reset();
	void initialize();
	void destroy();
	char * readLine(char * content, int numberOfBytes, FILE * endpoint);

};

#endif
