/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>
*/

#ifndef _DefragmentationLane_H
#define _DefragmentationLane_H

#include <memory/DefragmentationGroup.h>

/** 
 * the number of DefragmentationGroup per DefragmentationLane
 */
#define GROUPS_PER_LANE 1024

/**
 * A SmartPointer maps to a SmallSmartPointer inside
 * a DefragmentationGroup
 *
 * A DefragmentationGroup is in a DefragmentationLane.
 * A DefragmentationLane contains GROUPS_PER_LANE DefragmentationGroup.
 * DefragmentationLane are appended in a linked list.
 */
class DefragmentationLane{
	int m_number;
	/** list of DefragmentationGroup */
	DefragmentationGroup m_groups[GROUPS_PER_LANE];
	int m_fastGroup;
public:
	/** link to the next DefragmentationLane */
	void*m_next;

	SmallSmartPointer allocate(int n,int bytesPerElement,uint16_t*content,int*group);
	void constructor(int number,int bytesPerElement,bool show);
	bool canAllocate(int n,int bytesPerElement,bool show);
	void deallocate(SmallSmartPointer a);
	int getNumber();
	DefragmentationGroup*getGroup(int i);
};


#endif
