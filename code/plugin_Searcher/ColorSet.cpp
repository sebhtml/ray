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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

//#define CONFIG_DEBUG_VIRTUAL_COLORS

#include <plugin_Searcher/ColorSet.h>
#include <cryptography/crypto.h>

#include <stdint.h>
#include <iostream>
using namespace std;

#ifdef ASSERT
#include <assert.h>
#endif


/** Time complexity: constant **/
ColorSet::ColorSet(){
	VirtualKmerColor voidColor;
	m_virtualColors.push_back(voidColor);

	int i=0;

	OPERATION_NO_VIRTUAL_COLOR_HAS_PHYSICAL_COLOR_CREATION=i++;
	OPERATION_NO_VIRTUAL_COLOR_HAS_COUNT_CREATION=i++;
	OPERATION_NO_VIRTUAL_COLOR_HAS_HASH_CREATION=i++;
	OPERATION_VIRTUAL_COLOR_HAS_COLORS_FETCH=i++;
	OPERATION_NO_VIRTUAL_COLOR_HAS_COLORS_CREATION=i++;
	OPERATION_NEW_FROM_EMPTY=i++;
	OPERATION_NEW_FROM_SCRATCH=i++;
	OPERATION_applyHashOperation=i++;
	OPERATION_getHash=i++;
	OPERATION_getVirtualColorFrom=i++;
	OPERATION_createVirtualColorFrom =i++;
	OPERATION_incrementReferences =i++;
	OPERATION_decrementReferences =i++;
	OPERATION_purgeVirtualColor =i++;
	OPERATION_allocateVirtualColorHandle =i++;
	OPERATION_DUMMY =i++;


	for(int i=0;i<OPERATION_DUMMY;i++){
		m_operations[i]=0;
	}

	m_maximumHashOperations=16;
}


/** O(1) **/
void ColorSet::incrementReferences(VirtualKmerColorHandle handle){

	#ifdef ASSERT
	assert(handle < m_virtualColors.size());
	#endif

	// we don't care about the empty virtual color
	if(handle == 0)
		return;

	// it is no longer available...
	// because we will increment the references
	if(m_virtualColors[handle].getNumberOfReferences()==0){
		m_availableHandles.erase(handle);
	}

	m_virtualColors[handle].incrementReferences();

	#ifdef ASSERT
	assert(m_availableHandles.count(handle)==0);
	#endif

	m_operations[OPERATION_incrementReferences]++;
}

void ColorSet::decrementReferences(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	assert(handle < m_virtualColors.size());
	#endif

	// we don't care aboput the empty virtual color
	if(handle == 0)
		return;

	#ifdef ASSERT
	assert(m_availableHandles.count(handle)==0);
	assert(m_virtualColors[handle].getNumberOfReferences()>0);
	#endif

	m_virtualColors[handle].decrementReferences();

	// destroy the color if it is not used anymore
	// and if it is not the color 0
	if(handle>0 && m_virtualColors[handle].getNumberOfReferences()==0){
		purgeVirtualColor(handle);
	}

	m_operations[OPERATION_decrementReferences]++;
}

void ColorSet::purgeVirtualColor(VirtualKmerColorHandle handle){

	VirtualKmerColor*virtualColorToPurge=getVirtualColor(handle);
	
	uint64_t hashValue=virtualColorToPurge->getHash();

	int numberOfPhysicalColors=virtualColorToPurge->getNumberOfPhysicalColors();

	set<PhysicalKmerColor>*colors=virtualColorToPurge->getPhysicalColors();

	for(set<PhysicalKmerColor>::iterator i=colors->begin();
		i!=colors->end();i++){
		
		PhysicalKmerColor physicalColor=*i;

		#ifdef ASSERT
		assert(m_index.count(physicalColor)>0);
		assert(m_index[physicalColor].count(numberOfPhysicalColors)>0);
		assert(m_index[physicalColor][numberOfPhysicalColors].count(hashValue)>0);
		#endif

		m_index[physicalColor][numberOfPhysicalColors][hashValue].erase(handle);

		// No other has the same hash value
		if(m_index[physicalColor][numberOfPhysicalColors][hashValue].size()==0){
			m_index[physicalColor][numberOfPhysicalColors].erase(hashValue);
		}

		// no other has the same number of colors
		if(m_index[physicalColor][numberOfPhysicalColors].size()==0){
			m_index[physicalColor].erase(numberOfPhysicalColors);
		}

		// No other have this color.
		if(m_index[physicalColor].size()==0){
			m_index.erase(physicalColor);
		}
	}

	// erase all colors
	virtualColorToPurge->clear();

	// destroy it at will
	// actually, they are simply not removed.
	// instead, they are re-used.

	m_availableHandles.insert(handle);

	// but we don't remove it from the hash table
	// because it may be useful sometime
	// correction: we do remove it right now.

	#ifdef ASSERT
	assert(m_availableHandles.count(handle)>0);
	assert(virtualColorToPurge->getNumberOfPhysicalColors()==0);
	#endif

	m_operations[OPERATION_purgeVirtualColor]++;
}

/** O(1) **/
VirtualKmerColor*ColorSet::getVirtualColor(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	if(handle>=m_virtualColors.size())
		cout<<"Error, handle= "<<handle<<" but size= "<<m_virtualColors.size()<<endl;

	assert(handle< m_virtualColors.size());
	#endif

	return & m_virtualColors[handle];
}

int ColorSet::getTotalNumberOfPhysicalColors(){
	return m_physicalColors.size();
}

int ColorSet::getTotalNumberOfVirtualColors(){
	return m_virtualColors.size();
}

void ColorSet::printSummary(){
	cout<<endl;
	cout<<"**********************************************************"<<endl;
	cout<<"Coloring summary"<<endl;
	cout<<"  Number of virtual colors: "<<getTotalNumberOfVirtualColors()<<endl;
	cout<<"  Number of real colors: "<<getTotalNumberOfPhysicalColors()<<endl;
	cout<<endl;
	cout<<"Operations"<<endl;
	cout<<endl;

	cout<<"  OPERATION_getVirtualColorFrom operations: "<<m_operations[OPERATION_getVirtualColorFrom]<<endl;
	cout<<endl;
	cout<<"  OPERATION_NO_VIRTUAL_COLOR_HAS_PHYSICAL_COLOR_CREATION operations: "<<m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_PHYSICAL_COLOR_CREATION]<<endl;
	cout<<"  OPERATION_NO_VIRTUAL_COLOR_HAS_COUNT_CREATION operations: "<<m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_COUNT_CREATION]<<endl;
	cout<<"  OPERATION_NO_VIRTUAL_COLOR_HAS_HASH_CREATION operations: "<<m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_HASH_CREATION]<<endl;
	cout<<"  OPERATION_VIRTUAL_COLOR_HAS_COLORS_FETCH operations: "<<m_operations[OPERATION_VIRTUAL_COLOR_HAS_COLORS_FETCH]<<endl;
	cout<<"  OPERATION_NO_VIRTUAL_COLOR_HAS_COLORS_CREATION operations: "<<m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_COLORS_CREATION]<<endl;
	cout<<endl;
	cout<<"  OPERATION_createVirtualColorFrom  operations: "<<m_operations[OPERATION_createVirtualColorFrom]<<endl;
	cout<<endl;
	cout<<"  OPERATION_allocateVirtualColorHandle operations: "<<m_operations[OPERATION_allocateVirtualColorHandle]<<endl;
	cout<<"  OPERATION_NEW_FROM_EMPTY operations: "<<m_operations[OPERATION_NEW_FROM_EMPTY]<<endl;
	cout<<"  OPERATION_NEW_FROM_SCRATCH operations: "<<m_operations[OPERATION_NEW_FROM_SCRATCH]<<endl;
	cout<<endl;
	cout<<"  OPERATION_applyHashOperation operations: "<<m_operations[OPERATION_applyHashOperation]<<endl;
	cout<<"  OPERATION_getHash operations: "<<m_operations[OPERATION_getHash]<<endl;
	cout<<endl;
	cout<<"  OPERATION_incrementReferences  operations: "<<m_operations[OPERATION_incrementReferences]<<endl;
	cout<<"  OPERATION_decrementReferences  operations: "<<m_operations[OPERATION_decrementReferences]<<endl;
	cout<<endl;
	cout<<"  OPERATION_purgeVirtualColor  operations: "<<m_operations[OPERATION_purgeVirtualColor]<<endl;
	cout<<"**********************************************************"<<endl;
	cout<<endl;
}

void ColorSet::printColors(){

	for(int i=0;i<(int)m_virtualColors.size();i++){
		cout<<"Virtual color: "<<i<<endl;

		uint64_t references=m_virtualColors[i].getNumberOfReferences();
		cout<<" References: "<<references<<endl;

		set<PhysicalKmerColor>*colors=m_virtualColors[i].getPhysicalColors();
		cout<<" Number of physical colors: "<<colors->size()<<endl;
		cout<<" Physical colors: "<<endl;
		cout<<"  ";
		
		for(set<PhysicalKmerColor>::iterator j=colors->begin();j!=colors->end();j++){
			cout<<" "<<*j;
		}
		cout<<endl;

		if(colors->size()>0)
			cout<<endl;
	}

}

/** O(1) **/
VirtualKmerColorHandle ColorSet::allocateVirtualColorHandle(){
	// if there is a virtual color with 0 reference,
	// use it
	
	m_operations[OPERATION_allocateVirtualColorHandle]++;

	if(m_availableHandles.size()>0){

		VirtualKmerColorHandle handle= *(m_availableHandles.begin());

		// we don't remove it here from the available list.
		// instead, it will be removed from the avaialble list
		// when its references are > 0
	
		#ifdef ASSERT
		assert(m_virtualColors[handle].getNumberOfReferences()==0);
		assert(m_virtualColors[handle].getPhysicalColors()->size()==0);
		#endif

		// re-use a virtual color

		m_operations[OPERATION_NEW_FROM_EMPTY]++;

		return handle;
	}

	// otherwise, create a new one
	
	VirtualKmerColor newColor;
	m_virtualColors.push_back(newColor);

	VirtualKmerColorHandle handle=m_virtualColors.size()-1;

	// add it in the available list since it has 0 references
	m_availableHandles.insert(handle);

	m_operations[OPERATION_NEW_FROM_SCRATCH]++;

	return handle;
}

/** O(16) **/
uint64_t ColorSet::getHash(set<PhysicalKmerColor>*colors){
	if(colors->size()==0)
		return 0;

	uint64_t hashValue=*(colors->begin());

	int operations=0;
	
	for(set<PhysicalKmerColor>::iterator i=colors->begin();i!=colors->end();i++){
		PhysicalKmerColor color=*i;

		hashValue=applyHashOperation(hashValue,color);
	
		operations++;

		// don't hash more than N things
		if(operations==m_maximumHashOperations){
			break;
		}
	}

	#ifdef ASSERT
	assert(colors->size()>0);
	#endif

	m_operations[OPERATION_getHash]++;

	return hashValue;
}

uint64_t ColorSet::applyHashOperation(uint64_t hashValue,PhysicalKmerColor color){
	uint64_t localHash=uniform_hashing_function_1_64_64(color);
	hashValue ^= localHash;

	m_operations[OPERATION_applyHashOperation]++;

	return hashValue;
}

int ColorSet::getNumberOfReferences(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	assert(handle<m_virtualColors.size());
	#endif
	
	return m_virtualColors[handle].getNumberOfReferences();
}

int ColorSet::getNumberOfPhysicalColors(VirtualKmerColorHandle handle){

	#ifdef ASSERT
	assert(handle<m_virtualColors.size());
	#endif

	return m_virtualColors[handle].getNumberOfPhysicalColors();
}

VirtualKmerColorHandle ColorSet::getVirtualColorFrom(VirtualKmerColorHandle handle,PhysicalKmerColor color){

	#ifdef ASSERT
	assert(handle<m_virtualColors.size());
	assert(!m_virtualColors[handle].hasPhysicalColor(color));
	#endif

	m_operations[OPERATION_getVirtualColorFrom]++;

	m_physicalColors.insert(color);

	// case 1. no virtual color has the physical color
	if(m_index.count(color)==0){
		// no virtual color has this physical color...
		
		m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_PHYSICAL_COLOR_CREATION]++;

		return createVirtualColorFrom(handle,color);
	}

	// at this point, at least a virtual color has the physical color
	
	#ifdef ASSERT
	assert(m_index.count(color)>0);
	#endif

	int targetNumberOfColors=getNumberOfPhysicalColors(handle)+1;

	// see if any of them have the correct number of colors
	
	// case 2. no virtual color has the correct number of physical colors
	if(m_index[color].count(targetNumberOfColors)==0){

		// no virtual color with thte physical color
		// has the correct number of colors

		m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_COUNT_CREATION]++;

		return createVirtualColorFrom(handle,color);
	}

	#ifdef ASSERT
	assert(m_index[color].count(targetNumberOfColors)>0);
	#endif

	// check if any of them have the correct hash
	
	VirtualKmerColor*oldVirtualColor=getVirtualColor(handle);

	uint64_t expectedHash=oldVirtualColor->getHash();

	if(oldVirtualColor->getNumberOfPhysicalColors() < m_maximumHashOperations){
		expectedHash=applyHashOperation(expectedHash,color);
	}

	// case 3. no virtual color has the expected hash value
	if(m_index[color][targetNumberOfColors].count(expectedHash)==0){

		// at this point
		// at least one virtual color has the color and the correct number
		// of colors
		// however, none of them have a matching hash
	
		m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_HASH_CREATION]++;

		return createVirtualColorFrom(handle,color);
	}

	#ifdef ASSERT
	assert(m_index[color][targetNumberOfColors].count(expectedHash)>0);
	#endif

	// for each of the hits
	// check if they have all the required colors
	// if so, return it
	
	set<VirtualKmerColorHandle>*hits=&(m_index[color][targetNumberOfColors][expectedHash]);

	// case 4. a virtual color has:
	// (1) the color, 
	// (2) the correct number of physical colors,
	// (3) the expected hash value
	//
	// check it out
	for(set<VirtualKmerColorHandle>::iterator i=hits->begin();i!=hits->end();i++){
		VirtualKmerColorHandle virtualColorToInvestigate=*i;
		
		#ifdef ASSERT
		assert(virtualColorHasPhysicalColor(virtualColorToInvestigate,color));
		#endif

		if(virtualColorHasAllPhysicalColorsOf(virtualColorToInvestigate,handle)){
		
			m_operations[OPERATION_VIRTUAL_COLOR_HAS_COLORS_FETCH]++;
			return virtualColorToInvestigate;
		}
	}

	// at this point, we know for sure that no virtual color matches
	
	// case 5. no virtual color has all the required colors
	
	m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_COLORS_CREATION]++;

	return createVirtualColorFrom(handle,color);
}

VirtualKmerColorHandle ColorSet::createVirtualColorFrom(VirtualKmerColorHandle handle,PhysicalKmerColor color){
	VirtualKmerColorHandle newHandle=allocateVirtualColorHandle();

	#ifdef ASSERT
	assert(getNumberOfReferences(newHandle)==0);
	assert(getNumberOfPhysicalColors(newHandle)==0);
	#endif

	// add the colors and return it
	VirtualKmerColor*newVirtualColor=getVirtualColor(newHandle);

	newVirtualColor->copyPhysicalColors(getVirtualColor(handle));
	newVirtualColor->addPhysicalColor(color);

	set<PhysicalKmerColor>* colors=newVirtualColor->getPhysicalColors();
	int numberOfPhysicalColors=colors->size();

	uint64_t hashValue=getHash(colors);

	newVirtualColor->setHash(hashValue);

	// index it
	for(set<PhysicalKmerColor>::iterator i=colors->begin();i!=colors->end();i++){
	
		PhysicalKmerColor physicalColor=*i;
		m_index[physicalColor][numberOfPhysicalColors][hashValue].insert(newHandle);
	}

	m_operations[OPERATION_createVirtualColorFrom]++;

	return newHandle;
}

set<PhysicalKmerColor>*ColorSet::getPhysicalColors(VirtualKmerColorHandle handle){
	return getVirtualColor(handle)->getPhysicalColors();
}

bool ColorSet::virtualColorHasPhysicalColor(VirtualKmerColorHandle handle,PhysicalKmerColor color){
	return getVirtualColor(handle)->hasPhysicalColor(color);
}

bool ColorSet::virtualColorHasAllPhysicalColorsOf(VirtualKmerColorHandle toInvestigate,VirtualKmerColorHandle list){
	return getVirtualColor(toInvestigate)->virtualColorHasAllPhysicalColorsOf(getVirtualColor(list));
}
