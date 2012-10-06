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

#include <plugin_Searcher/VirtualKmerColor.h>

#include <stdint.h>
#include <vector>
#include <map>
#include <iostream>
using namespace std;

#define NULL_VIRTUAL_COLOR 0

// maximum value for a uint64_t:
// 18446744073709551615
// xxxxyyyyyyyyyyyyyyyy
//    10000000000000000
#define COLOR_NAMESPACE_MULTIPLIER 10000000000000000ULL

// colors in the phylogeny namespace: 1024 * 10000000000000000 + identifier 
#define COLOR_NAMESPACE_PHYLOGENY 	1024
#define COLOR_NAMESPACE_EMBL_CDS 	789

typedef uint32_t VirtualKmerColorHandle;

/** This class is a translation table for
 * allocated virtual colors. 
 *
 * This is the Flyweight design pattern.
 *
 * \author: Sébastien Boisvert
 *
 * Frédéric Raymond proposed the idea of using color namespaces.
 */
class ColorSet{

	int OPERATION_IN_PLACE_ONE_REFERENCE;
	int OPERATION_NO_VIRTUAL_COLOR_HAS_PHYSICAL_COLOR_CREATION;
	int OPERATION_NO_VIRTUAL_COLOR_HAS_COUNT_CREATION;
	int OPERATION_NO_VIRTUAL_COLOR_HAS_HASH_CREATION;
	int OPERATION_VIRTUAL_COLOR_HAS_COLORS_FETCH;
	int OPERATION_NO_VIRTUAL_COLOR_HAS_COLORS_CREATION;
	int OPERATION_NEW_FROM_EMPTY;
	int OPERATION_NEW_FROM_SCRATCH;
	int OPERATION_applyHashOperation;
	int OPERATION_getHash;
	int OPERATION_getVirtualColorFrom;
	int OPERATION_createVirtualColorFrom ;
	int OPERATION_incrementReferences ;
	int OPERATION_decrementReferences ;
	int OPERATION_purgeVirtualColor ;
	int OPERATION_allocateVirtualColorHandle ;
	int OPERATION_DUMMY ;

	LargeCount m_operations[32];

/** a list of available handles **/
	set<VirtualKmerColorHandle> m_availableHandles;

/** the table of virtual colors **/
	vector<VirtualKmerColor> m_virtualColors;

/** a list of physical colors **/
	set<PhysicalKmerColor> m_physicalColors;

	map<LargeIndex,set<VirtualKmerColorHandle> > m_index;

	LargeCount m_collisions;



/** get the hash value for a set of colors **/
	uint64_t getHash(set<PhysicalKmerColor>*colors);

/** allocates a virtual color handle **/
	VirtualKmerColorHandle allocateVirtualColorHandle();

	VirtualKmerColor*getVirtualColor(VirtualKmerColorHandle handle);


	void purgeVirtualColor(VirtualKmerColorHandle handle);

	uint64_t applyHashOperation(uint64_t hashValue,PhysicalKmerColor color);

	VirtualKmerColorHandle createVirtualColorFrom(VirtualKmerColorHandle handle,PhysicalKmerColor color);

	bool virtualColorHasAllPhysicalColorsOf(VirtualKmerColorHandle toInvestigate,VirtualKmerColorHandle list);

	void addVirtualColorToIndex(VirtualKmerColorHandle handle);
	void removeVirtualColorFromIndex(VirtualKmerColorHandle handle);

	VirtualKmerColorHandle createVirtualColorHandleFromScratch();

	void assertNoVirtualColorDuplicates(VirtualKmerColorHandle handle,PhysicalKmerColor color,int id);

	void printPhysicalColors(set<PhysicalKmerColor>*colors);
	VirtualKmerColorHandle lookupVirtualColor(set<PhysicalKmerColor>*colors);
	VirtualKmerColorHandle createVirtualColorFromPhysicalColors(set<PhysicalKmerColor>*colors);
public:
	
	ColorSet();

/** increment the references for a virtual color **/
	void incrementReferences(VirtualKmerColorHandle handle);

/** decrement the references for a virtual color **/
	void decrementReferences(VirtualKmerColorHandle handle);
	
	LargeCount getNumberOfReferences(VirtualKmerColorHandle handle);

	int getNumberOfPhysicalColors(VirtualKmerColorHandle handle);

	VirtualKmerColorHandle getVirtualColorFrom(VirtualKmerColorHandle handle,PhysicalKmerColor color);

	int getTotalNumberOfPhysicalColors();
	LargeCount getTotalNumberOfVirtualColors();

	void printSummary(ostream*out,bool xml);
	void printColors(ostream*out);

	set<PhysicalKmerColor>*getPhysicalColors(VirtualKmerColorHandle handle);

	bool virtualColorHasPhysicalColor(VirtualKmerColorHandle handle,PhysicalKmerColor color);

/*
 * Returns a virtual color with exactly the physical colors provided.
 */
	VirtualKmerColorHandle findVirtualColor(set<PhysicalKmerColor>*colors);
};

#endif

