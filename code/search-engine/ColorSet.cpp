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

#include <search-engine/ColorSet.h>
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
	m_translationTable.push_back(voidColor);

	m_hashDirectOperations=0;
	m_hashSizeOperations=0;
	m_hashBruteForceOperations=0;
	m_newFromOldOperations=0;
	m_newOperations=0;
}

/** Time complexity: 
 *
 * - O(1)	if no other has the same hash value
 *
 * - O(number of virtual colors with the same hash)
 *   		if other have the same hash, but not the correct number of colors
 *
 * - O(sum of physical colors of virtual colors)
 *   		if more than one has the same hash and the same number of colors
 *
 * - O(1) 	if not in the table but virtual colors are available
 *
 * - O(1)	if not in the table and no virtual colors are available
 */
VirtualKmerColorHandle ColorSet::getVirtualColorHandle(set<PhysicalKmerColor>*colors){

	// first, try to find it with the fast-access table
	
	uint64_t hashValue=getHash(colors);

	// verify the hash
	if(m_fastAccessTable.count(hashValue)){

		#ifdef ASSERT
		assert(m_fastAccessTable[hashValue].size()>0);
		#endif

		// if there is only one virtual color, it is easy
		
		VirtualKmerColorHandle onlyChoice=*(m_fastAccessTable[hashValue].begin());

		// if only 1 has the number of colors
		// return it
		if(m_fastAccessTable[hashValue].size()==1 // only 1
			&& m_translationTable[onlyChoice].getColors()->size() == colors->size() // same size
			&& m_translationTable[onlyChoice].hasColors(colors)){ // same colors

			m_hashDirectOperations++;

			return *(m_fastAccessTable[hashValue].begin());
		}else{

			#ifdef ASSERT
			assert(m_fastAccessTable[hashValue].size()>=1);
			#endif

			VirtualKmerColorHandle selection=*(m_fastAccessTable[hashValue].begin());
			int numberOfHits=0;

			// try to find one with the correct number of colors
			for(set<VirtualKmerColorHandle>::iterator i=m_fastAccessTable[hashValue].begin();
				i!=m_fastAccessTable[hashValue].end();i++){
				
				VirtualKmerColorHandle possibleChoice=*i;
	
				if(m_translationTable[possibleChoice].getColors()->size() == colors->size()){
					selection=possibleChoice;
	
					numberOfHits++;
				}
			}
	
			// only one has the correct number of physical colors
			if(numberOfHits==1 
				&& m_translationTable[selection].getColors()->size() == colors->size() // same size
				&& m_translationTable[selection].hasColors(colors)){ // same colors
	
				m_hashSizeOperations++;
	
				return selection;
			}else{
	
				#ifdef ASSERT
				assert(numberOfHits>=0);
				#endif
		
				// otherwise try to match the count and  the colors
				for(set<VirtualKmerColorHandle>::iterator i=m_fastAccessTable[hashValue].begin();
					i!=m_fastAccessTable[hashValue].end();i++){
					
					VirtualKmerColorHandle possibleChoice=*i;
		
					if(m_translationTable[possibleChoice].getColors()->size() == colors->size() // same count
					&&  m_translationTable[possibleChoice].hasColors(colors)){ // same colors
		
						m_hashBruteForceOperations++;
		
						return possibleChoice;
					}
				}
			}
		}
	}
	
	// otherwise, we need to add a new color and index it

	int oldSize=m_translationTable.size();

	VirtualKmerColorHandle newVirtualColor=allocateVirtualColor();

	if((int)m_translationTable.size()==oldSize){ // we re-used an old virtual color
		m_newFromOldOperations++;
	}else{
		m_newOperations++; // we created a new virtual color
	}

	#ifdef ASSERT
	assert(newVirtualColor>=0 && newVirtualColor <m_translationTable.size());
	assert(m_translationTable[newVirtualColor].getColors()->size()==0);
	assert(m_translationTable[newVirtualColor].getReferences()==0);
	assert(m_availableHandles.count(newVirtualColor) > 0);
	#endif
	
	// add physical colors
	for(set<PhysicalKmerColor>::iterator i=colors->begin();i!=colors->end();i++){

		PhysicalKmerColor physicalColor=*i;
		m_translationTable[newVirtualColor].addPhysicalColor(physicalColor);

		m_physicalColors.insert(physicalColor);
	}

	#ifdef ASSERT
	if(m_fastAccessTable.count(hashValue)>0){
		assert(m_fastAccessTable[hashValue].count(newVirtualColor)==0);
	}
	#endif

	// remove collision
	// because we use a set
	m_fastAccessTable[hashValue].insert(newVirtualColor); 
	
	// set the hash value
	m_translationTable[newVirtualColor].setHash(hashValue);

	#ifdef CONFIG_DEBUG_VIRTUAL_COLORS
	cout<<"Created a new color!"<<endl;
	cout<<"Physical colors: "<<endl;
	for(set<PhysicalKmerColor>::iterator i=colors->begin();i!=colors->end();i++){
		cout<<" "<<*i;
	}
	cout<<endl;

	cout<<"Now with "<<getNumberOfVirtualColors()<<" virtual colors for "<<getNumberOfPhysicalColors()<<" physical colors."<<endl;

	if(getNumberOfVirtualColors()%1000==0)
		printSummary();

	#endif

	#ifdef ASSERT
	assert(m_fastAccessTable.count(hashValue)>0);
	assert(m_fastAccessTable[hashValue].size()>0);
	assert(m_fastAccessTable[hashValue].count(newVirtualColor)>0);
	assert(hashValue==m_translationTable[newVirtualColor].getHash());
	#endif

	return newVirtualColor;
}


/** O(1) **/
void ColorSet::incrementReferences(VirtualKmerColorHandle handle){

	#ifdef ASSERT
	assert(handle < m_translationTable.size());
	#endif

	// we don't care about the empty virtual color
	if(handle == 0)
		return;

	// it is no longer available...
	// because we will increment the references
	if(m_translationTable[handle].getReferences()==0){
		m_availableHandles.erase(handle);
	}

	m_translationTable[handle].incrementReferences();

	#ifdef ASSERT
	assert(m_availableHandles.count(handle)==0);
	#endif
}

void ColorSet::decrementReferences(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	assert(handle < m_translationTable.size());
	#endif

	// we don't care aboput the empty virtual color
	if(handle == 0)
		return;

	#ifdef ASSERT
	assert(m_availableHandles.count(handle)==0);
	assert(m_translationTable[handle].getReferences()>0);
	#endif

	m_translationTable[handle].decrementReferences();

	// destroy the color if it is not used anymore
	// and if it is not the color 0
	if(handle>0 && m_translationTable[handle].getReferences()==0){

		// erase all colors
		m_translationTable[handle].getColors()->clear();
		
		// destroy it at will
		// actually, they are simply not removed.
		// instead, they are re-used.

		m_availableHandles.insert(handle);

		// but we don't remove it from the hash table
		// because it may be useful sometime
		// correction: we do remove it right now.
	
		uint64_t hashValue=m_translationTable[handle].getHash();

		#ifdef ASSERT
		assert(m_fastAccessTable.count(hashValue)>0);
		assert(m_fastAccessTable[hashValue].count(handle)>0);
		#endif

		m_fastAccessTable[hashValue].erase(handle);

		// remove the entry from the table if it was the last
		if(m_fastAccessTable[hashValue].size()==0){
			m_fastAccessTable.erase(hashValue);
		}

		#ifdef ASSERT
		if(m_fastAccessTable.count(hashValue)>0){
			assert(m_fastAccessTable[hashValue].count(handle)==0);
		}

		assert(m_availableHandles.count(handle)>0);
		assert(m_translationTable[handle].getColors()->size()==0);
		#endif
	}
}

/** O(1) **/
VirtualKmerColor*ColorSet::getVirtualColor(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	if(handle>=m_translationTable.size())
		cout<<"Error, handle= "<<handle<<" but size= "<<m_translationTable.size()<<endl;

	assert(handle< m_translationTable.size());
	#endif

	return & m_translationTable[handle];
}

int ColorSet::getNumberOfPhysicalColors(){
	return m_physicalColors.size();
}

int ColorSet::getNumberOfVirtualColors(){
	return m_translationTable.size();
}

void ColorSet::printSummary(){
	cout<<endl;
	cout<<"Coloring summary"<<endl;
	cout<<"  Number of virtual colors: "<<getNumberOfVirtualColors()<<endl;
	cout<<"  Number of real colors: "<<getNumberOfPhysicalColors()<<endl;
	cout<<endl;
	cout<<"  Operations"<<endl;
	cout<<"    Fetched with direct fast access: "<<m_hashDirectOperations<<endl;
	cout<<"    Fetched with size fast access: "<<m_hashSizeOperations<<endl;
	cout<<"    Fetched with brute-force fast access: "<<m_hashBruteForceOperations<<endl;
	cout<<"    Fetched new from old: "<<m_newFromOldOperations<<endl;
	cout<<"    Fetched new: "<<m_newOperations<<endl;
	cout<<endl;
}

void ColorSet::printColors(){

	for(int i=0;i<(int)m_translationTable.size();i++){
		cout<<"Virtual color: "<<i<<endl;

		uint64_t references=m_translationTable[i].getReferences();
		cout<<" References: "<<references<<endl;

		set<PhysicalKmerColor>*colors=m_translationTable[i].getColors();
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
VirtualKmerColorHandle ColorSet::allocateVirtualColor(){
	// if there is a virtual color with 0 reference,
	// use it
	
	if(m_availableHandles.size()>0){

		VirtualKmerColorHandle handle= *(m_availableHandles.begin());

		// we don't remove it here from the available list.
		// instead, it will be removed from the avaialble list
		// when its references are > 0
	
		#ifdef ASSERT
		assert(m_translationTable[handle].getReferences()==0);
		assert(m_translationTable[handle].getColors()->size()==0);
		#endif

		// re-use a virtual color
		return handle;
	}

	// otherwise, create a new one
	
	VirtualKmerColor newColor;
	m_translationTable.push_back(newColor);

	VirtualKmerColorHandle handle=m_translationTable.size()-1;

	// add it in the available list since it has 0 references
	m_availableHandles.insert(handle);

	return handle;
}

/** O(16) **/
uint64_t ColorSet::getHash(set<PhysicalKmerColor>*colors){
	if(colors->size()==0)
		return 0;

	uint64_t hashValue=*(colors->begin());

	int operations=0;
	int maximumOperations=16;

	for(set<PhysicalKmerColor>::iterator i=colors->begin();i!=colors->end();i++){
		PhysicalKmerColor color=*i;

		uint64_t localHash=uniform_hashing_function_1_64_64(color);
		hashValue ^= localHash;
		
		operations++;

		// don't hash more than N things
		if(operations==maximumOperations){
			break;
		}
	}

	#ifdef ASSERT
	assert(colors->size()>0);
	#endif

	return hashValue;
}

