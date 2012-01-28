/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _Message_H
#define _Message_H

#include <stdint.h>
#include <core/types.h>

/**
 * In Ray, every message is a Message.
 * the inbox and the outbox are arrays of Message's
 * All the code in Ray utilise Message to communicate information.
 * MPI_Datatype is always MPI_UNSIGNED_LONG_LONG
 * m_count is >=0 and <= MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)
 *  (default is 4000/8 = 500 ).
 * \author Sébastien Boisvert
 */
class Message{
	/** the message body, contains data
 * 	if NULL, m_count must be 0 */
	uint64_t*m_buffer;

	/** the number of uint64_t that the m_buffer contains 
 * 	can be 0 regardless of m_buffer value
 * 	*/
	int m_count;

	/** the Message-passing interface rank destination 
 * 	Must be >=0 and <= MPI_Comm_size()-1 */
	Rank m_destination;

	/**
 * 	Ray message-passing interface message tags are named RAY_MPI_TAG_<something>
 * 	see mpi_tag_macros.h 
 */
	MessageTag m_tag;

	/** the message-passing interface rank source 
 * 	Must be >=0 and <= MPI_Comm_size()-1 */
	Rank m_source;
public:
	Message();
	Message(uint64_t*b,int c,Rank dest,MessageTag tag,Rank source);
	uint64_t*getBuffer();
	int getCount();
/**
 * Returns the destination MPI rank
 */
	Rank getDestination();

/**
 * Returns the message tag (RAY_MPI_TAG_something)
 */
	MessageTag getTag();
/**
 * Gets the source MPI rank
 */
	Rank getSource();

	void print();

	void setBuffer(uint64_t*buffer);

	void setTag(MessageTag tag);

	void setSource(Rank source);

	void setDestination(Rank destination);
};

#endif
