/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <search-engine/VirtualKmerColor.h>

#ifdef ASSERT
#include <assert.h>
#endif

VirtualKmerColor::VirtualKmerColor(){
	m_references=0;
}

void VirtualKmerColor::incrementReferences(){
	m_references++;
}

void VirtualKmerColor::decrementReferences(){
	m_references--;

	#ifdef ASSERT
	assert(m_references>=0);
	#endif
}

void VirtualKmerColor::addPhysicalColor(PhysicalKmerColor color){
	m_colors.push_back(color);
}

uint64_t VirtualKmerColor::getReferences(){
	return m_references;
}
