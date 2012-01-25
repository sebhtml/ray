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

#include <search-engine/ColorSet.h>

#ifdef ASSERT
#include <assert.h>
#endif

ColorSet::ColorSet(){
	VirtualKmerColor voidColor;
	m_translationTable.push_back(voidColor);

}

VirtualKmerColorHandle ColorSet::getVirtualColor(vector<PhysicalKmerColor>*colors){
	// try to find the color
	
	map<VirtualKmerColorHandle,int> counts;

	int numberOfColors=colors->size();

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
		if(count==numberOfColors)
			return handle;
	}

	// otherwise, we need to add a new color and index it

	VirtualKmerColor newColor;
	
	// add physical colors
	for(int i=0;i<numberOfColors;i++){
		newColor.addPhysicalColor(colors->at(i));
	}

	m_translationTable.push_back(newColor);

	return m_translationTable.size()-1;
}


void ColorSet::incrementReferences(VirtualKmerColorHandle handle){

	#ifdef ASSERT
	assert(handle < m_translationTable.size());
	#endif

	m_translationTable[handle].incrementReferences();
}

void ColorSet::decrementReferences(VirtualKmerColorHandle handle){
	#ifdef ASSERT
	assert(handle < m_translationTable.size());
	#endif

	m_translationTable[handle].decrementReferences();

	// destroy the color if it is not used anymore
	// and if it is not the color 0
	if(handle>0 && m_translationTable[handle].getReferences()==0){
		
		// destroy it at will
	}
}
