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

#include <handlers/SlaveModeHandler.h>
#ifdef ASSERT
#include <assert.h>
#endif
#include <stdlib.h> /* for NULL */

// define empty implementations
//
#define ITEM(mode) \
void SlaveModeHandler::call_ ## mode (){}

#include <scripting/slave_modes.txt>

#undef ITEM

void SlaveModeHandler::callHandler(SlaveMode mode){
	SlaveModeHandler*object=m_objects[mode];

	#ifdef ASSERT
	assert(object!=NULL);
	#endif

	// don't call it if it is this
	if(object==this)
		return;

	// otherwise, fetch the method
	SlaveModeHandlerMethod method=m_methods[mode];

	#ifdef ASSERT
	assert(method!=NULL);
	#endif

	// call it
	(object->*method) (   );
}

SlaveModeHandler::SlaveModeHandler(){
	// assign the methods and the default object handlers
	// the default is this for the objects
	#define ITEM(mode) \
	setMethodHandler(mode, &SlaveModeHandler::call_ ## mode ); \
	setObjectHandler(mode, this ); 

	#include <scripting/slave_modes.txt>

	#undef ITEM

}

void SlaveModeHandler::setObjectHandler(SlaveMode mode,SlaveModeHandler*object){
	#ifdef ASSERT
	assert(object!=NULL);
	#endif

	m_objects[mode]=object;
}

void SlaveModeHandler::setMethodHandler(SlaveMode mode,SlaveModeHandlerMethod method){
	#ifdef ASSERT
	assert(method!=NULL);
	#endif

	m_methods[mode]=method;
}
