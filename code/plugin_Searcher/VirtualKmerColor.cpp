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
#include <iostream>
using namespace std;
#ifdef ASSERT
#include <assert.h>
#endif

VirtualKmerColor::VirtualKmerColor(){
	clear();
}

void VirtualKmerColor::clear(){
	
	m_references=0;
	m_colors.clear();
	m_hash=0;

	#ifdef ASSERT
	assert(getNumberOfReferences()==0);
	#endif

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

	#ifdef ASSERT
	assert(!hasPhysicalColor(color));
	#endif

	m_colors.insert(color);
}

LargeCount VirtualKmerColor::getNumberOfReferences(){
	return m_references;
}

set<PhysicalKmerColor>*VirtualKmerColor::getPhysicalColors(){
	return & m_colors;
}

bool VirtualKmerColor::hasPhysicalColor(PhysicalKmerColor color){
	return m_colors.count(color)>0;
}

bool VirtualKmerColor::hasPhysicalColors(set<PhysicalKmerColor>*colors){

	// verify the colors
	for(set<PhysicalKmerColor>::iterator i=colors->begin();
		i!=colors->end();i++){

		PhysicalKmerColor physicalColor=*i;

		if(!hasPhysicalColor(physicalColor)){
			return false;
		}
	}

	return true;
}

void VirtualKmerColor::setHash(uint64_t hash){
	m_hash=hash;
}

uint64_t VirtualKmerColor::getCachedHashValue(){
	return m_hash;
}

int VirtualKmerColor::getNumberOfPhysicalColors(){
	return getPhysicalColors()->size();
}

bool VirtualKmerColor::virtualColorHasAllPhysicalColorsOf(VirtualKmerColor*a,PhysicalKmerColor color){
	
	// we are searching for a virtual color with only the physical color <color> and 
	// the physical colors of VirtualKmerColor <a>

	// this don't have the physical color <color>
	if(!hasPhysicalColor(color)){
		//cout<<"bah don't have color "<<color<<endl;
		//
		return false;
	}

	// this does not have the correct number of physical colors
	if((a->getNumberOfPhysicalColors()+1) != getNumberOfPhysicalColors()){
/*
		cout<<" mew wrong count"<<endl;
		cout<<"Expected: "<<(a->getNumberOfPhysicalColors()+1)<<" Actual: "<<getNumberOfPhysicalColors()<<endl;
		cout<<"a->getNumberOfPhysicalColors()-> "<<a->getNumberOfPhysicalColors()<<endl;
*/

		return false;
	}

	// we don't need to check the hash because it was done
	// upstream already, hopefully...
	// anyway, the class VirtualKmerColor does not have the methods 
	// to do hash checking
	
	// the current virtual color does not have all the required 
	// physical colors
	if(!hasPhysicalColors(a->getPhysicalColors())){
		//cout<<"ah don't have all physical colors"<<endl;
		//
		return false;
	}

	return true;
}

void VirtualKmerColor::copyPhysicalColors(VirtualKmerColor*a){
	set<PhysicalKmerColor>*colors=a->getPhysicalColors();

	addPhysicalColors(colors);
}

void VirtualKmerColor::addPhysicalColors(set<PhysicalKmerColor>*colors){

	for(set<PhysicalKmerColor>::iterator i=colors->begin();
		i!=colors->end();i++){

		PhysicalKmerColor color=*i;

		addPhysicalColor(color);
	}
}
