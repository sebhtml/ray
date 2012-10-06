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
//#define ASSERT_LOW_LEVEL

#include <plugin_Searcher/ColorSet.h>
#include <cryptography/crypto.h>

#include <stdint.h>
#include <iostream>
using namespace std;

#ifdef ASSERT_LOW_LEVEL
#ifndef ASSERT
#define ASSERT
#endif /* ASSERT */
#endif /* ASSERT_LOW_LEVEL */

#ifdef ASSERT
#include <assert.h>
#endif /* ASSERT */

/** Time complexity: constant **/
ColorSet::ColorSet(){
	VirtualKmerColorHandle voidColor=createVirtualColorHandleFromScratch();

	getVirtualColor(voidColor)->clear();

	int i=0;
	
	OPERATION_IN_PLACE_ONE_REFERENCE=i++;
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

	m_collisions=0;
}

/** O(1) **/
void ColorSet::incrementReferences(VirtualKmerColorHandle handle){

	#ifdef ASSERT
	assert(handle < getTotalNumberOfVirtualColors());
	#endif

	// we don't care about the empty virtual color
	if(handle == NULL_VIRTUAL_COLOR)
		return;

	VirtualKmerColor*virtualColor=getVirtualColor(handle);

	// it is no longer available...
	// because we will increment the references
	if(virtualColor->getNumberOfReferences()==0){
		m_availableHandles.erase(handle);
	}

	virtualColor->incrementReferences();

	#ifdef ASSERT
	assert(m_availableHandles.count(handle)==0);
	assert(getVirtualColor(handle)->getNumberOfReferences()>=1);
	#endif

	m_operations[OPERATION_incrementReferences]++;
}

void ColorSet::decrementReferences(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	assert(handle < getTotalNumberOfVirtualColors());
	#endif

	VirtualKmerColor*virtualColor=getVirtualColor(handle);

	// we don't care aboput the empty virtual color
	if(handle == NULL_VIRTUAL_COLOR)
		return;

	#ifdef ASSERT
	assert(m_availableHandles.count(handle)==0);
	assert(virtualColor->getNumberOfReferences()>0);
	#endif

	virtualColor->decrementReferences();

	// destroy the color if it is not used anymore
	// and if it is not the color 0
	if(handle!=NULL_VIRTUAL_COLOR && virtualColor->getNumberOfReferences()==0){
		purgeVirtualColor(handle);
	}

	m_operations[OPERATION_decrementReferences]++;
}

void ColorSet::purgeVirtualColor(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	assert(getVirtualColor(handle)->getNumberOfReferences()==0);
	#endif

	VirtualKmerColor*virtualColorToPurge=getVirtualColor(handle);

	removeVirtualColorFromIndex(handle);

	// erase all colors
	// also sets references to 0 
	// and resets the hash value
	virtualColorToPurge->clear();

	// destroy it at will
	// actually, they are simply not removed.
	// instead, they are re-used.

	#ifdef ASSERT
	assert(m_availableHandles.count(handle)==0);
	#endif

	m_availableHandles.insert(handle);

	// but we don't remove it from the hash table
	// because it may be useful sometime
	// correction: we do remove it right now.

	#ifdef ASSERT
	assert(m_availableHandles.count(handle)>0);
	assert(virtualColorToPurge->getNumberOfPhysicalColors()==0);
	assert(virtualColorToPurge->getNumberOfReferences()==0);
	#endif

	m_operations[OPERATION_purgeVirtualColor]++;
}

/** O(1) **/
VirtualKmerColor*ColorSet::getVirtualColor(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	if(handle>= getTotalNumberOfVirtualColors())
		cout<<"Error, handle= "<<handle<<" but size= "<<getTotalNumberOfVirtualColors()<<endl;

	assert(handle< getTotalNumberOfVirtualColors());
	#endif

	return & m_virtualColors[handle];
}

int ColorSet::getTotalNumberOfPhysicalColors(){
	return m_physicalColors.size();
}

LargeCount ColorSet::getTotalNumberOfVirtualColors(){
	return m_virtualColors.size();
}

void ColorSet::printSummary(ostream*out,bool xml){

	(*out)<<endl;
	(*out)<<"**********************************************************"<<endl;
	(*out)<<"Coloring summary"<<endl;
	(*out)<<"  Number of virtual colors: "<<getTotalNumberOfVirtualColors()<<endl;
	(*out)<<"  Number of real colors: "<<getTotalNumberOfPhysicalColors()<<endl;
	(*out)<<endl;
	(*out)<<"Keys in index: "<<m_index.size()<<endl;
	(*out)<<"Observed collisions when populating the index: "<<m_collisions<<endl;
	(*out)<<"COLOR_NAMESPACE_MULTIPLIER= "<<COLOR_NAMESPACE_MULTIPLIER<<endl;
	(*out)<<endl;

	(*out)<<"Operations"<<endl;
	(*out)<<endl;

	(*out)<<"  OPERATION_getVirtualColorFrom operations: "<<m_operations[OPERATION_getVirtualColorFrom]<<endl;
	(*out)<<endl;
	(*out)<<"  OPERATION_IN_PLACE_ONE_REFERENCE: "<<m_operations[OPERATION_IN_PLACE_ONE_REFERENCE]<<endl;
	(*out)<<"  OPERATION_NO_VIRTUAL_COLOR_HAS_HASH_CREATION operations: "<<m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_HASH_CREATION]<<endl;
	(*out)<<"  OPERATION_VIRTUAL_COLOR_HAS_COLORS_FETCH operations: "<<m_operations[OPERATION_VIRTUAL_COLOR_HAS_COLORS_FETCH]<<endl;
	(*out)<<"  OPERATION_NO_VIRTUAL_COLOR_HAS_COLORS_CREATION operations: "<<m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_COLORS_CREATION]<<endl;
	(*out)<<endl;
	(*out)<<"  OPERATION_createVirtualColorFrom  operations: "<<m_operations[OPERATION_createVirtualColorFrom]<<endl;
	(*out)<<endl;
	(*out)<<"  OPERATION_allocateVirtualColorHandle operations: "<<m_operations[OPERATION_allocateVirtualColorHandle]<<endl;
	(*out)<<"  OPERATION_NEW_FROM_EMPTY operations: "<<m_operations[OPERATION_NEW_FROM_EMPTY]<<endl;
	(*out)<<"  OPERATION_NEW_FROM_SCRATCH operations: "<<m_operations[OPERATION_NEW_FROM_SCRATCH]<<endl;
	(*out)<<endl;
	(*out)<<"  OPERATION_applyHashOperation operations: "<<m_operations[OPERATION_applyHashOperation]<<endl;
	(*out)<<"  OPERATION_getHash operations: "<<m_operations[OPERATION_getHash]<<endl;
	(*out)<<endl;
	(*out)<<"  OPERATION_incrementReferences  operations: "<<m_operations[OPERATION_incrementReferences]<<endl;
	(*out)<<"  OPERATION_decrementReferences  operations: "<<m_operations[OPERATION_decrementReferences]<<endl;
	(*out)<<endl;
	(*out)<<"  OPERATION_purgeVirtualColor  operations: "<<m_operations[OPERATION_purgeVirtualColor]<<endl;
	(*out)<<"**********************************************************"<<endl;
	(*out)<<endl;

	if(!xml){
		return;
	}

	// print frequencies
	
	map<CoverageDepth,LargeCount>  referenceFrequencies;
	map<CoverageDepth,LargeCount> colorFrequencies;


	for(int i=0;i< (int)getTotalNumberOfVirtualColors();i++){
		LargeCount references=getVirtualColor(i)->getNumberOfReferences();

		referenceFrequencies[references]++;

		set<PhysicalKmerColor>*colors=getVirtualColor(i)->getPhysicalColors();

		colorFrequencies[colors->size()]++;
	}

	(*out)<<endl;
	(*out)<<"<referencesPerVirtualColor><frequencies>"<<endl;

	for(map<CoverageDepth,LargeCount>::iterator i=referenceFrequencies.begin();i!=referenceFrequencies.end();
		i++){
		
		(*out)<<i->first<<"	"<<i->second<<endl;
	}
	(*out)<<"</frequencies></referencesPerVirtualColor>"<<endl;

	(*out)<<"<physicalColorsPerVirtualColor><frequencies>"<<endl;

	for(map<CoverageDepth,LargeCount>::iterator i=colorFrequencies.begin();i!=colorFrequencies.end();
		i++){
		
		(*out)<<i->first<<"	"<<i->second<<endl;
	}
	(*out)<<"</frequencies></physicalColorsPerVirtualColor>"<<endl;
}

void ColorSet::printColors(ostream*out){

	(*out)<<"Coloring summary"<<endl;
	(*out)<<"  Number of virtual colors: "<<getTotalNumberOfVirtualColors()<<endl;
	(*out)<<"  Number of real colors: "<<getTotalNumberOfPhysicalColors()<<endl;
	(*out)<<endl;

	for(int i=0;i< (int)getTotalNumberOfVirtualColors();i++){
		(*out)<<"Virtual color: "<<i<<endl;

		LargeCount references=getVirtualColor(i)->getNumberOfReferences();
		(*out)<<" References: "<<references<<endl;

		set<PhysicalKmerColor>*colors=getVirtualColor(i)->getPhysicalColors();
		(*out)<<" Number of physical colors: "<<colors->size()<<endl;
		(*out)<<" Physical colors: "<<endl;
		(*out)<<"  ";
		
		for(set<PhysicalKmerColor>::iterator j=colors->begin();j!=colors->end();j++){
			(*out)<<" "<<*j;
		}
		(*out)<<endl;

		if(colors->size()>0)
			(*out)<<endl;
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
		assert(getVirtualColor(handle)->getNumberOfReferences()==0);
		assert(getVirtualColor(handle)->getPhysicalColors()->size()==0);
		#endif

		// re-use a virtual color

		m_operations[OPERATION_NEW_FROM_EMPTY]++;

		return handle;
	}

	#ifdef ASSERT
	assert(m_availableHandles.size()==0);
	#endif

	// otherwise, create a new one
	
	VirtualKmerColorHandle handle=createVirtualColorHandleFromScratch();

	// add it in the available list since it has 0 references
	m_availableHandles.insert(handle);

	m_operations[OPERATION_NEW_FROM_SCRATCH]++;

	#ifdef ASSERT
	assert(m_availableHandles.size()==1);
	#endif

	return handle;
}

/** O(16) **/
uint64_t ColorSet::getHash(set<PhysicalKmerColor>*colors){

	uint64_t hashValue=0;
	
	for(set<PhysicalKmerColor>::iterator i=colors->begin();i!=colors->end();i++){

		PhysicalKmerColor color=*i;
		hashValue=applyHashOperation(hashValue,color);
	}

	m_operations[OPERATION_getHash]++;

	return hashValue;
}

uint64_t ColorSet::applyHashOperation(uint64_t hashValue,PhysicalKmerColor color){

	uint64_t localHash=1;

	int operations=10;
	while(operations--){
		localHash*=color;
	}

	hashValue += localHash;

	m_operations[OPERATION_applyHashOperation]++;

	return hashValue;
}

LargeCount ColorSet::getNumberOfReferences(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	assert(handle< getTotalNumberOfVirtualColors());
	#endif
	
	return getVirtualColor(handle)->getNumberOfReferences();
}

int ColorSet::getNumberOfPhysicalColors(VirtualKmerColorHandle handle){

	#ifdef ASSERT
	assert(handle< getTotalNumberOfVirtualColors());
	#endif

	return getVirtualColor(handle)->getNumberOfPhysicalColors();
}

VirtualKmerColorHandle ColorSet::getVirtualColorFrom(VirtualKmerColorHandle handle,PhysicalKmerColor color){

	#ifdef ASSERT
	assert(handle<getTotalNumberOfVirtualColors());
	assert(!getVirtualColor(handle)->hasPhysicalColor(color));
	assert(m_availableHandles.count(handle)==0);
	#endif

	m_operations[OPERATION_getVirtualColorFrom]++;

	m_physicalColors.insert(color);

	VirtualKmerColor*oldVirtualColor=getVirtualColor(handle);
	// check if any of them have the correct hash
	
	uint64_t oldHash=oldVirtualColor->getCachedHashValue();

	// in any case, we have to compute the hash from scratch
	// because the new color has to be inserted in
	// a sorted se
	// 2012-02-01: not anymore, captain !
	// the sum of a group of numbers does not depend on their order...
	
	uint64_t expectedHash=applyHashOperation(oldHash,color);

	// case 3. no virtual color has the expected hash value
	if(m_index.count(expectedHash)==0){

		// case X.: the virtual color has only one reference
		// in that case, we can just add the color to it and
		// update its hash...
		// but before doing that, we need to check that no one has the expectedHash
	
		if(oldVirtualColor->getNumberOfReferences()==1){
			// we can update it, no problem
			// because nobody is using it
			// it is the copy-on-write design pattern I guess

			removeVirtualColorFromIndex(handle);

			oldVirtualColor->addPhysicalColor(color);
			oldVirtualColor->setHash(expectedHash);

			addVirtualColorToIndex(handle);

			m_operations[OPERATION_IN_PLACE_ONE_REFERENCE]++;

			return handle;
		}


		// at this point
		// at least one virtual color has the color and the correct number
		// of colors
		// however, none of them have a matching hash
	
		//cout<<" No one has has value= "<<expectedHash<<endl;

		m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_HASH_CREATION]++;

		#ifdef ASSERT_LOW_LEVEL
		assertNoVirtualColorDuplicates(handle,color,433);
		#endif /* ASSERT_LOW_LEVEL */
	
		return createVirtualColorFrom(handle,color);
	}

	#ifdef ASSERT
	assert(m_index.count(expectedHash)>0);
	assert(m_index[expectedHash].size()>=1);
	#endif /* ASSERT */

	// for each of the hits
	// check if they have all the required colors
	// if so, return it
	
	set<VirtualKmerColorHandle>*hits=&(m_index[expectedHash]);

	//cout<<"hits "<<hits->size()<<endl;

	// case 4. a virtual color has:
	// (1) the color, 
	// (2) the correct number of physical colors,
	// (3) the expected hash value
	//
	// check it out
	for(set<VirtualKmerColorHandle>::iterator i=hits->begin();i!=hits->end();i++){
		VirtualKmerColorHandle virtualColorToInvestigate=*i;
		
		VirtualKmerColor*toCheck=getVirtualColor(virtualColorToInvestigate);

		//cout<<"Checking virtual color "<<virtualColorToInvestigate<<endl;

		if(toCheck->virtualColorHasAllPhysicalColorsOf(oldVirtualColor,color)){

			#ifdef ASSERT
			assert(virtualColorHasPhysicalColor(virtualColorToInvestigate,color));
			#endif /* ASSERT */
	
			m_operations[OPERATION_VIRTUAL_COLOR_HAS_COLORS_FETCH]++;
			return virtualColorToInvestigate;
		}
	}

	// at this point, we know for sure that no virtual color matches
	
	// case 5. no virtual color has all the required colors
	
	m_operations[OPERATION_NO_VIRTUAL_COLOR_HAS_COLORS_CREATION]++;

	#ifdef ASSERT_LOW_LEVEL
	assertNoVirtualColorDuplicates(handle,color,479);
	#endif

	return createVirtualColorFrom(handle,color);
}

VirtualKmerColorHandle ColorSet::createVirtualColorFrom(VirtualKmerColorHandle handle,PhysicalKmerColor color){

	VirtualKmerColorHandle newHandle=allocateVirtualColorHandle();

	// consume the handle
	m_availableHandles.erase(newHandle);

	#ifdef ASSERT
	assert(!getVirtualColor(handle)->hasPhysicalColor(color));
	assert(m_availableHandles.count(newHandle)==0);
	assert(getNumberOfReferences(newHandle)==0);
	assert(getNumberOfPhysicalColors(newHandle)==0);
	#endif /* ASSERT */

	// add the colors and return it
	VirtualKmerColor*newVirtualColor=getVirtualColor(newHandle);

	newVirtualColor->copyPhysicalColors(getVirtualColor(handle));
	newVirtualColor->addPhysicalColor(color);

	VirtualKmerColor*oldVirtualColor=getVirtualColor(handle);

	uint64_t oldHash=oldVirtualColor->getCachedHashValue();
	uint64_t hashValue=applyHashOperation(oldHash,color);

	newVirtualColor->setHash(hashValue);

	addVirtualColorToIndex(newHandle);

	m_operations[OPERATION_createVirtualColorFrom]++;

	#ifdef ASSERT
	assert(getVirtualColor(newHandle)->getNumberOfPhysicalColors()==getVirtualColor(handle)->getNumberOfPhysicalColors()+1);
	assert(getVirtualColor(newHandle)->getNumberOfReferences()==0);
	assert(getVirtualColor(newHandle)->virtualColorHasAllPhysicalColorsOf(oldVirtualColor,color));
	#endif

	return newHandle;
}

set<PhysicalKmerColor>*ColorSet::getPhysicalColors(VirtualKmerColorHandle handle){
	return getVirtualColor(handle)->getPhysicalColors();
}

bool ColorSet::virtualColorHasPhysicalColor(VirtualKmerColorHandle handle,PhysicalKmerColor color){
	return getVirtualColor(handle)->hasPhysicalColor(color);
}

void ColorSet::removeVirtualColorFromIndex(VirtualKmerColorHandle handle){

	VirtualKmerColor*virtualColor=getVirtualColor(handle);

	uint64_t hashValue=virtualColor->getCachedHashValue();

	m_index[hashValue].erase(handle);

	if(m_index[hashValue].size()==0)
		m_index.erase(hashValue);
}

void ColorSet::addVirtualColorToIndex(VirtualKmerColorHandle handle){

	VirtualKmerColor*virtualColor=getVirtualColor(handle);

	uint64_t hashValue=virtualColor->getCachedHashValue();

	m_index[hashValue].insert(handle);
}

VirtualKmerColorHandle ColorSet::createVirtualColorHandleFromScratch(){
	VirtualKmerColor a;
	m_virtualColors.push_back(a);

	VirtualKmerColorHandle handle=getTotalNumberOfVirtualColors()-1;
	getVirtualColor(handle)->clear();

	return handle;
}

void ColorSet::assertNoVirtualColorDuplicates(VirtualKmerColorHandle handle,PhysicalKmerColor color,int caseX){
	set<PhysicalKmerColor> desiredColors;
	set<PhysicalKmerColor>*colors3=getVirtualColor(handle)->getPhysicalColors();
	for(set<PhysicalKmerColor>::iterator i=colors3->begin();i!=colors3->end();i++){
		desiredColors.insert(*i);
	}
	desiredColors.insert(color);

	for(int i=0;i<(int)getTotalNumberOfVirtualColors();i++){
		if(getVirtualColor(i)->getNumberOfPhysicalColors()==(int)desiredColors.size()){
			if(getVirtualColor(i)->hasPhysicalColors(&desiredColors)){
				cout<<"Error, there is a virtual color that does the job already, case= "<<caseX<<endl;
				cout<<"virtual color "<<i<<" has "<<getVirtualColor(i)->getNumberOfPhysicalColors()<<" physical colors";
				cout<<" and "<<getVirtualColor(i)->getNumberOfReferences()<<" references"<<endl;
				printPhysicalColors(getVirtualColor(i)->getPhysicalColors());
				cout<<"Searched for "<<getVirtualColor(handle)->getNumberOfPhysicalColors()+1<<" physical colors,"<<endl;
				printPhysicalColors(&desiredColors);
				cout<<"previous virtual color was ";
				cout<<handle<<" with exactly "<<getVirtualColor(handle)->getNumberOfPhysicalColors()<<" physical colors"<<endl;

				printPhysicalColors(getVirtualColor(handle)->getPhysicalColors());
			}

			#ifdef ASSERT
			assert(!getVirtualColor(i)->hasPhysicalColors(&desiredColors));
			#endif /* ASSERT */
		}
	}
		
}

void ColorSet::printPhysicalColors(set<PhysicalKmerColor>*colors3){

	for(set<PhysicalKmerColor>::iterator i=colors3->begin();i!=colors3->end();i++){
		cout<<" "<<*i;
	}
	cout<<endl;
}

VirtualKmerColorHandle ColorSet::findVirtualColor(set<PhysicalKmerColor>*colors){

	if(colors->size()==0)
		return NULL_VIRTUAL_COLOR;

	VirtualKmerColorHandle virtualColorInStore=lookupVirtualColor(colors);

	if(virtualColorInStore!=NULL_VIRTUAL_COLOR)
		return virtualColorInStore;

	
/*
 * At this point, we need to create a virtual color with all the required 
 * physical colors.
 */

	return createVirtualColorFromPhysicalColors(colors);
}

VirtualKmerColorHandle ColorSet::createVirtualColorFromPhysicalColors(set<PhysicalKmerColor>*colors){

	if(colors->size()==0)
		return NULL_VIRTUAL_COLOR;
	
	#ifdef ASSERT
	assert(colors->size()!=0);
	#endif /* ASSERT */

	VirtualKmerColorHandle newHandle=allocateVirtualColorHandle();

	#ifdef ASSERT
	assert(m_availableHandles.count(newHandle)>0);
	#endif /* ASSERT */

	m_availableHandles.erase(newHandle);

	VirtualKmerColor*newVirtualColor=getVirtualColor(newHandle);

	newVirtualColor->addPhysicalColors(colors);

	uint64_t expectedHash=getHash(colors);
	newVirtualColor->setHash(expectedHash);
	addVirtualColorToIndex(newHandle);

	#ifdef ASSERT
	assert(lookupVirtualColor(colors)!=NULL_VIRTUAL_COLOR);
	#endif /* ASSERT */

	return newHandle;
}

VirtualKmerColorHandle ColorSet::lookupVirtualColor(set<PhysicalKmerColor>*colors){
	uint64_t expectedHash=getHash(colors);

	if(m_index.count(expectedHash)>0){
		set<VirtualKmerColorHandle>*hits=&(m_index[expectedHash]);

		for(set<VirtualKmerColorHandle>::iterator i=hits->begin();i!=hits->end();i++){

			VirtualKmerColorHandle virtualColorToInvestigate=*i;
		
			VirtualKmerColor*toCheck=getVirtualColor(virtualColorToInvestigate);

/*
 * We need the same number of physical colors.
 */
			if((int)colors->size()!=toCheck->getNumberOfPhysicalColors())
				continue;

/*
 * Each physical color must match 
 */
			if(!toCheck->hasPhysicalColors(colors))
				continue;

/*
 * The matching virtual color was found.
 */
			return virtualColorToInvestigate;
		}
	}

	return NULL_VIRTUAL_COLOR;
}
