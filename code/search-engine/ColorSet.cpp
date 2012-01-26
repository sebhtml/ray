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
			for(int j=0;j<(int)m_index[physicalColor].size();j++){
				VirtualKmerColorHandle handle=m_index[physicalColor][j];

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

	VirtualKmerColor newColor;
	
	VirtualKmerColorHandle newVirtualColor=m_translationTable.size();

	// add physical colors
	for(int i=0;i<numberOfColors;i++){
		PhysicalKmerColor physicalColor=colors->at(i);
		newColor.addPhysicalColor(physicalColor);

		// index it
		m_index[physicalColor].push_back(newVirtualColor);
	}

	// add the new virtual color
	m_translationTable.push_back(newColor);


	#ifdef CONFIG_DEBUG_VIRTUAL_COLORS
	cout<<"Created a new color!"<<endl;
	cout<<"Physical colors: "<<endl;
	for(int i=0;i<(int)colors->size();i++){
		cout<<" "<<colors->at(i);
	}
	cout<<endl;

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
