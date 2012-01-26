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

ColorSet::ColorSet(){
	VirtualKmerColor voidColor;
	m_translationTable.push_back(voidColor);

	m_hashOperations=0;
	m_bruteOperations=0;
	m_newFromOldOperations=0;
	m_newOperations=0;
}

VirtualKmerColorHandle ColorSet::getVirtualColorHandle(set<PhysicalKmerColor>*colors){

	// first, try to find it with the fast-access table
	
	uint64_t hashValue=getHash(colors);

	// verify the hash
	if(m_fastAccessTable.count(hashValue)){
		for(set<VirtualKmerColorHandle>::iterator i=m_fastAccessTable[hashValue].begin();
			i!=m_fastAccessTable[hashValue].end();i++){
			
			VirtualKmerColorHandle possibleChoice=*i;

			if(m_translationTable[possibleChoice].hasColors(colors)){

				m_hashOperations++;

				return possibleChoice;
			}
		}
	}

	// otherwise, we need to add a new color and index it

	int oldSize=m_translationTable.size();

	VirtualKmerColorHandle newVirtualColor=allocateVirtualColor();

	if(m_translationTable.size()==oldSize){
		m_newFromOldOperations++;
	}else{
		m_newOperations++;
	}

	#ifdef ASSERT
	assert(newVirtualColor>=0 && newVirtualColor <m_translationTable.size());
	assert(m_translationTable[newVirtualColor].getColors()->size()==0);
	assert(m_translationTable[newVirtualColor].getReferences()==0);
	#endif
	
	// add physical colors
	for(set<PhysicalKmerColor>::iterator i=colors->begin();i!=colors->end();i++){
		PhysicalKmerColor physicalColor=*i;
		m_translationTable[newVirtualColor].addPhysicalColor(physicalColor);

		m_physicalColors.insert(physicalColor);
	}
	
	m_fastAccessTable[hashValue].insert(newVirtualColor); // remove collision

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

	return newVirtualColor;
}


void ColorSet::incrementReferences(VirtualKmerColorHandle handle){

	#ifdef ASSERT
	assert(handle < m_translationTable.size());
	#endif

	// we don't care aboput the empty virtual color
	if(handle == 0)
		return;

	m_translationTable[handle].incrementReferences();
}

void ColorSet::decrementReferences(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	assert(handle < m_translationTable.size());
	#endif

	// we don't care aboput the empty virtual color
	if(handle == 0)
		return;

	m_translationTable[handle].decrementReferences();

	// destroy the color if it is not used anymore
	// and if it is not the color 0
	if(handle>0 && m_translationTable[handle].getReferences()==0){
		
		// destroy it at will
		// actually, they are simply not removed.
		// instead, they are re-used.
	}
}

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
	cout<<"Number of real colors: "<<getNumberOfPhysicalColors()<<endl;
	cout<<"Number of virtual colors: "<<getNumberOfVirtualColors()<<endl;
	cout<<endl;
	cout<<"Operations"<<endl;
	cout<<" Fetched with fast access: "<<m_hashOperations<<endl;
	cout<<" Fetched with brute-force: "<<m_bruteOperations<<endl;
	cout<<" Fetched new from old: "<<m_newFromOldOperations<<endl;
	cout<<" Fetched new: "<<m_newOperations<<endl;
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

VirtualKmerColorHandle ColorSet::allocateVirtualColor(){
	// if there is a virtual color with 0 reference,
	// use it
	
	for(VirtualKmerColorHandle i=1;i<m_translationTable.size();i++){
		if(m_translationTable[i].getReferences()==0){
			set<PhysicalKmerColor>*oldColors=m_translationTable[i].getColors();
			for(set<PhysicalKmerColor>::iterator j=oldColors->begin();
				j!=oldColors->end();j++){
				PhysicalKmerColor oldColor=*j;
			}

			uint64_t hashValue=getHash(m_translationTable[i].getColors());

			// remove the entry from the fast access table
			if(m_fastAccessTable.count(hashValue)>0 && m_fastAccessTable[hashValue].count(i) > 0){
				m_fastAccessTable[hashValue].erase(hashValue);

				if(m_fastAccessTable[hashValue].size()==0){
					m_fastAccessTable.erase(hashValue);
				}
			}

			// erase all colors
			m_translationTable[i].getColors()->clear();

			// re-use a virtual color
			return i;
		}
	}

	// otherwise, create a new one
	
	VirtualKmerColor newColor;
	m_translationTable.push_back(newColor);

	return m_translationTable.size()-1;
}

uint64_t ColorSet::getHash(set<PhysicalKmerColor>*colors){
	if(colors->size()==0)
		return 0;

	uint64_t hashValue=*(colors->begin());

	int operations=0;

	for(set<PhysicalKmerColor>::iterator i=colors->begin();i!=colors->end();i++){
		PhysicalKmerColor color=*i;

		uint64_t localHash=uniform_hashing_function_1_64_64(color);
		hashValue ^= localHash;
		
		operations++;

		// don't hash more than 8 things
		if(operations==8)
			break;
	}

	#ifdef ASSERT
	assert(colors->size()>0);
	#endif

	return hashValue;
}

