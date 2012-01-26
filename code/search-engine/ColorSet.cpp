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

#define CONFIG_DEBUG_VIRTUAL_COLORS

#include <search-engine/ColorSet.h>

#include <iostream>
using namespace std;

#ifdef ASSERT
#include <assert.h>
#endif

ColorSet::ColorSet(){
	VirtualKmerColor voidColor;
	m_translationTable.push_back(voidColor);

}

VirtualKmerColorHandle ColorSet::getVirtualColorHandle(vector<PhysicalKmerColor>*colors){
	// try to find the color
	
	map<VirtualKmerColorHandle,int> counts;

	int numberOfColors=colors->size();

	// try to find a color with the characteristics
	for(int i=0;i<numberOfColors;i++){
		PhysicalKmerColor physicalColor=colors->at(i);

		if(m_index.count(physicalColor)>0){
			for(set<VirtualKmerColorHandle>::iterator j=m_index[physicalColor].begin();j!=m_index[physicalColor].end();j++){
				VirtualKmerColorHandle handle= *j;

				counts[handle]++;
			}
		}
	}

	for(map<VirtualKmerColorHandle,int>::iterator i=counts.begin();
		i!=counts.end();i++){
		
		VirtualKmerColorHandle handle=i->first;
		int count=i->second;

		// we found it
		if(count==numberOfColors
			 && (int)getVirtualColor(handle)->getColors()->size() == numberOfColors){

			return handle;
		}
	}

	// otherwise, we need to add a new color and index it

	VirtualKmerColorHandle newVirtualColor=allocateVirtualColor();

	#ifdef ASSERT
	assert(newVirtualColor>=0 && newVirtualColor <m_translationTable.size());
	assert(m_translationTable[newVirtualColor].getColors()->size()==0);
	assert(m_translationTable[newVirtualColor].getReferences()==0);
	#endif
	
	// add physical colors
	for(int i=0;i<numberOfColors;i++){
		PhysicalKmerColor physicalColor=colors->at(i);
		m_translationTable[newVirtualColor].addPhysicalColor(physicalColor);

		// index it
		m_index[physicalColor].insert(newVirtualColor);
	}

	#ifdef CONFIG_DEBUG_VIRTUAL_COLORS
	cout<<"Created a new color!"<<endl;
	cout<<"Physical colors: "<<endl;
	for(int i=0;i<(int)colors->size();i++){
		cout<<" "<<colors->at(i);
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
	return m_index.size();
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
	for(int i=0;i<(int)m_translationTable.size();i++){
		cout<<"Virtual color: "<<i<<endl;
		set<PhysicalKmerColor>*colors=m_translationTable[i].getColors();
		uint64_t references=m_translationTable[i].getReferences();
		cout<<" Number of physical colors: "<<colors->size()<<endl;
		cout<<" References: "<<references<<endl;
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

				if(m_index.count(oldColor) > 0){
					m_index[oldColor].erase(i);
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
