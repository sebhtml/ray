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

#ifndef _ColorSet_h
#define _ColorSet_h

#include <search-engine/VirtualKmerColor.h>

#include <stdint.h>
#include <vector>
#include <map>
using namespace std;

typedef uint32_t VirtualKmerColorHandle;

/** This class is a translation table for
 * allocated virtual colors. 
 */
class ColorSet{

	uint64_t m_hashOperations;
	uint64_t m_bruteOperations;
	uint64_t m_newFromOldOperations;
	uint64_t m_newOperations;

/** the table of virtual colors **/
	vector<VirtualKmerColor> m_translationTable;

	set<PhysicalKmerColor> m_physicalColors;

	VirtualKmerColorHandle allocateVirtualColor();

/** fast access table **/
	map<uint64_t,set<VirtualKmerColorHandle> > m_fastAccessTable;

	uint64_t getHash(set<PhysicalKmerColor>*colors);

public:
	
	ColorSet();

/** get a handle to a virtual color */
	VirtualKmerColorHandle getVirtualColorHandle(set<PhysicalKmerColor>*colors);

	void incrementReferences(VirtualKmerColorHandle handle);
	void decrementReferences(VirtualKmerColorHandle handle);
	
	VirtualKmerColor*getVirtualColor(VirtualKmerColorHandle handle);

	int getNumberOfPhysicalColors();
	int getNumberOfVirtualColors();

	void printSummary();
	void printColors();
};

#endif

