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

#include <handlers/MessageTagHandler.h>
#include <communication/mpi_tags.h>
#ifdef ASSERT
#include <assert.h>
#endif
#include <stdlib.h> /* for NULL */

void MessageTagHandler::callHandler(MessageTag messageTag,Message*message){
	MessageTagHandler*handlerObject=m_objects[messageTag];

	// it is useless to call base implementations
	// because they are empty
	if(handlerObject==NULL)
		return;

	handlerObject->call(message);
}

MessageTagHandler::MessageTagHandler(){
	for(int i=0;i<MAXIMUM_NUMBER_OF_TAG_HANDLERS;i++){
		m_objects[i]=NULL;
	}
}

void MessageTagHandler::setObjectHandler(MessageTag messageTag,MessageTagHandler*object){
	m_objects[messageTag]=object;
}

void MessageTagHandler::call(Message*message){
}
