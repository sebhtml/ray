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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _Message_H
#define _Message_H

#include <mpi.h>

/*
 * In Ray, every message is a Message.
 * the inbox and the outbox are arrays of Message's
 * All the code in Ray utilise Message to communicate information.
 * MPI_Datatype is always MPI_UNSIGNED_LONG_LONG
 * m_count is >=0 and <= MAXIMUM_MESSAGE_SIZE_IN_BYTES (default is 4000).
 */
class Message{
	void*m_buffer;
	int m_count;
	MPI_Datatype m_datatype;
	int m_dest;
	int m_tag;
	int m_source;
public:
	Message();
	Message(void*b,int c,MPI_Datatype d,int dest,int tag,int source);
	void*getBuffer();
	int getCount();
	MPI_Datatype getMPIDatatype();
/**
 * Returns the destination MPI rank
 */
	int getDestination();
/**
 * Returns the message tag (RAY_MPI_TAG_something)
 */
	int getTag();
/**
 * Gets the source MPI rank
 */
	int getSource();
	void setBuffer(void*buffer);
};

#endif
