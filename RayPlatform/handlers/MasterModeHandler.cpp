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

#include <handlers/MasterModeHandler.h>
#include <communication/mpi_tags.h>
#include <core/types.h>
#include <stdlib.h> /* for NULL */

#ifdef ASSERT
#include <assert.h>
#endif

void MasterModeHandler::callHandler(MasterMode mode){
	MasterModeHandler*object=m_objects[mode];

	// don't do it if it is NULL because it does nothing
	if(object==NULL)
		return;

	/** otherwise, fetch the method and call it*/

	object->call();
}

MasterModeHandler::MasterModeHandler(){
	for(int i=0;i<MAXIMUM_NUMBER_OF_MASTER_HANDLERS;i++){
		m_objects[i]=NULL;
	}
}

void MasterModeHandler::setObjectHandler(MasterMode mode,MasterModeHandler*object){
	m_objects[mode]=object;
}

void MasterModeHandler::call(){
}

