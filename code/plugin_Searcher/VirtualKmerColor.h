/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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


#ifndef _VirtualKmerColor_h
#define _VirtualKmerColor_h

#include <application_core/constants.h>
#include <stdint.h>
#include <vector>
#include <set>
using namespace std;

/** a physical color contains its namespace **/
typedef uint64_t PhysicalKmerColor;

/**
 * An implementation of a virtual color type.
 * A virtual color can be translated to a set of physical colors.
 *
 * This class utilises the Flyweight design pattern.
 *
 * \author: Sébastien Boisvert
 */
class VirtualKmerColor{

/** 
 * half the number of k-mers using this virtual color 
 */
	LargeCount m_references;

/**
 * the list of physical colors 
 */
	set<PhysicalKmerColor> m_colors;

	uint64_t m_hash;

public:
	VirtualKmerColor();

	void addPhysicalColor(PhysicalKmerColor color);
	void addPhysicalColors(set<PhysicalKmerColor>*color);

	void incrementReferences();
	void decrementReferences();

	LargeCount getNumberOfReferences();

	set<PhysicalKmerColor>*getPhysicalColors();

	bool hasPhysicalColor(PhysicalKmerColor color);

	bool hasPhysicalColors(set<PhysicalKmerColor>*colors);

	void setHash(uint64_t hash);
	uint64_t getCachedHashValue();

	int getNumberOfPhysicalColors();

	void clear();

	bool virtualColorHasAllPhysicalColorsOf(VirtualKmerColor*a,PhysicalKmerColor color);

	void copyPhysicalColors(VirtualKmerColor*a);
};

#endif
