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
using namespace std;

// maximum value for a uint64_t:
// 18446744073709551615
// xxxx000yyyyyyyyyyyyy
//    10000000000000000
#define COLOR_NAMESPACE 10000000000000000

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

/** number of direct access operations **/
	uint64_t m_hashDirectOperations;

/** number of size-related hash operations **/
	uint64_t m_hashSizeOperations;

/** number of brute-force hash operations **/
	uint64_t m_hashBruteForceOperations;

/** number of recycling operations **/
	uint64_t m_newFromOldOperations;

/** number of creations **/
	uint64_t m_newOperations;

/** a list of available handles **/
	set<VirtualKmerColorHandle> m_availableHandles;

/** the table of virtual colors **/
	vector<VirtualKmerColor> m_translationTable;

/** a list of physical colors **/
	set<PhysicalKmerColor> m_physicalColors;

/** allocates a virtual color handle **/
	VirtualKmerColorHandle allocateVirtualColor();

/** fast access table **/
	map<uint64_t,set<VirtualKmerColorHandle> > m_fastAccessTable;

/** get the hash value for a set of colors **/
	uint64_t getHash(set<PhysicalKmerColor>*colors);

public:
	
	ColorSet();

/** get a handle to a virtual color */
	VirtualKmerColorHandle getVirtualColorHandle(set<PhysicalKmerColor>*colors);

/** increment the references for a virtual color **/
	void incrementReferences(VirtualKmerColorHandle handle);

/** decrement the references for a virtual color **/
	void decrementReferences(VirtualKmerColorHandle handle);
	
	VirtualKmerColor*getVirtualColor(VirtualKmerColorHandle handle);

	int getNumberOfPhysicalColors();
	int getNumberOfVirtualColors();

	void printSummary();
	void printColors();
};

#endif

