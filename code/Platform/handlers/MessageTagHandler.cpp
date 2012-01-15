/*
 	Ray
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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>
*/

#include <handlers/MessageTagHandler.h>
#include <communication/mpi_tags.h>
#ifdef ASSERT
#include <assert.h>
#endif
#include <stdlib.h> /* for NULL */

// define empty implementations
// these are virtual and can be overwritten
#define ITEM(tag) \
void MessageTagHandler::call_ ## tag(Message*message){}

#include <scripting/mpi_tags.txt>

#undef ITEM

void MessageTagHandler::callHandler(Tag messageTag,Message*message){
	MessageTagHandler*handlerObject=m_objects[messageTag];

	#ifdef ASSERT
	assert(handlerObject!=NULL);
	#endif

	// it is useless to call base implementations
	// because they are empty
	if(handlerObject==this)
		return;

	// otherwise, fetch the method and call it
	MessageTagHandlerMethod handlerMethod=m_methods[messageTag];

	#ifdef ASSERT
	assert(handlerMethod!=NULL);
	#endif

	(handlerObject->*handlerMethod)(message);
}

MessageTagHandler::MessageTagHandler(){
	// assign handler methods and default handler objects
	#define ITEM(tag) \
	setMethodHandler(tag, & MessageTagHandler::call_ ## tag); \
	setObjectHandler(tag, this);

	#include <scripting/mpi_tags.txt>

	#undef ITEM
}

void MessageTagHandler::setObjectHandler(Tag messageTag,MessageTagHandler*object){
	#ifdef ASSERT
	assert(object!=NULL);
	#endif

	m_objects[messageTag]=object;
}

void MessageTagHandler::setMethodHandler(Tag tag,MessageTagHandlerMethod method){
	#ifdef ASSERT
	assert(method!=NULL);
	#endif

	m_methods[tag]=method;
}
