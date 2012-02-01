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

#include <plugin_Searcher/VirtualKmerColor.h>

#ifdef ASSERT
#include <assert.h>
#endif

VirtualKmerColor::VirtualKmerColor(){
	m_references=0;
}

void VirtualKmerColor::clear(){
	
	#ifdef ASSERT
	assert(getNumberOfReferences()==0);
	#endif

	m_colors.clear();
	m_hash=0;

	#ifdef ASSERT
	assert(getNumberOfPhysicalColors()==0);
	#endif
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
	m_colors.insert(color);
}

uint64_t VirtualKmerColor::getNumberOfReferences(){
	return m_references;
}

set<PhysicalKmerColor>*VirtualKmerColor::getPhysicalColors(){
	return & m_colors;
}

bool VirtualKmerColor::hasPhysicalColor(PhysicalKmerColor color){
	return m_colors.count(color)>0;
}

bool VirtualKmerColor::hasPhysicalColors(set<PhysicalKmerColor>*colors){
	if((int)colors->size()!=getNumberOfPhysicalColors()){
		return false;
	}

	// verify the count
	if((int)colors->size()==getNumberOfPhysicalColors()){
		bool correct=true;

		// verify the colors
		for(set<PhysicalKmerColor>::iterator i=colors->begin();
			i!=colors->end();i++){

			PhysicalKmerColor physicalColor=*i;

			if(!hasPhysicalColor(physicalColor)){
				correct=false;
				break;
			}
		}

		// we found the correct choice already !
		if(correct){
			return true;
		}
	}

	return false;
}

void VirtualKmerColor::setHash(uint64_t hash){
	m_hash=hash;
}

uint64_t VirtualKmerColor::getCachedHashValue(){
	return m_hash;
}

int VirtualKmerColor::getNumberOfPhysicalColors(){
	return m_colors.size();
}

bool VirtualKmerColor::virtualColorHasAllPhysicalColorsOf(VirtualKmerColor*a){
	set<PhysicalKmerColor>*colors=a->getPhysicalColors();

	for(set<PhysicalKmerColor>::iterator i=colors->begin();
		i!=colors->end();i++){
		
		PhysicalKmerColor color=*i;

		if(!hasPhysicalColor(color))
			return false;
	}

	return true;
}

void VirtualKmerColor::copyPhysicalColors(VirtualKmerColor*a){
	set<PhysicalKmerColor>*colors=a->getPhysicalColors();

	for(set<PhysicalKmerColor>::iterator i=colors->begin();
		i!=colors->end();i++){

		PhysicalKmerColor color=*i;

		addPhysicalColor(color);
	}
}
