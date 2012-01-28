/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#ifndef _MessageTagHandler_h
#define _MessageTagHandler_h

#include <communication/Message.h>
#include <core/types.h>
#include <communication/mpi_tags.h>

/**
 * base class for handling message tags
 * \author Sébastien Boisvert
 * with help from Élénie Godzaridis for the design
 */
class MessageTagHandler{

/** table of object handlers */
	MessageTagHandler*m_objects[MAXIMUM_NUMBER_OF_TAG_HANDLERS];

public:

	virtual void call(Message*message);

	/** call the correct handler for a tag on a message */
	void callHandler(MessageTag messageTag,Message*message);

/** set the object to call for a given tag */
	void setObjectHandler(MessageTag messageTag,MessageTagHandler*object);

/** set default object and method handlers */
	MessageTagHandler();
};

#endif
