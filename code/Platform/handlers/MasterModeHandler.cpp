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

#include <handlers/MasterModeHandler.h>
#include <communication/mpi_tags.h>
#include <core/types.h>
#include <stdlib.h> /* for NULL */

#ifdef ASSERT
#include <assert.h>
#endif

#define ITEM(mode) \
void MasterModeHandler::call_ ## mode (){}

#include <scripting/master_modes.txt>

#undef ITEM

void MasterModeHandler::callHandler(MasterMode mode){
	MasterModeHandler*object=m_objects[mode];

	#ifdef ASSERT
	assert(object!=NULL);
	#endif

	// don't do it if it is this because it does nothing
	if(object==this)
		return;

	/** otherwise, fetch the method and call it*/

	MasterModeHandlerMethod method=m_methods[mode];

	#ifdef ASSERT
	assert(method!=NULL);
	#endif

	(object->*method)();
}

MasterModeHandler::MasterModeHandler(){
	// assign handler methods
	// also assign default handler objects

	#define ITEM(mode) \
	setMethodHandler(mode, &MasterModeHandler::call_ ## mode); \
	setObjectHandler(mode, this) ;

	#include <scripting/master_modes.txt>

	#undef ITEM
}

void MasterModeHandler::setObjectHandler(MasterMode mode,MasterModeHandler*object){
	#ifdef ASSERT
	assert(object!=NULL);
	#endif

	m_objects[mode]=object;
}

void MasterModeHandler::setMethodHandler(MasterMode mode,MasterModeHandlerMethod method){
	#ifdef ASSERT
	assert(method!=NULL);
	#endif
	m_methods[mode]=method;
}
