/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#ifndef _DefragmentationLane_H
#define _DefragmentationLane_H

#include <memory/DefragmentationGroup.h>

/** value for a invalid DefragmentationGroup */
#define INVALID_GROUP 123467

/** 
 * the number of DefragmentationGroup per DefragmentationLane
 */
#define GROUPS_PER_LANE 1024

/** the number of DefragmentationGroup */
#define NUMBER_OF_FAST_GROUPS 128

/**
 * A SmartPointer maps to a SmallSmartPointer inside
 * a DefragmentationGroup
 *
 * A DefragmentationGroup is in a DefragmentationLane.
 * A DefragmentationLane contains GROUPS_PER_LANE DefragmentationGroup.
 * \author Sébastien Boisvert
 */
class DefragmentationLane{
	/** the identifier of the DefragmentationLane */
	int m_number;

	/** list of DefragmentationGroup */
	DefragmentationGroup m_groups[GROUPS_PER_LANE];

	/** fast DefragmentationGroup objects */
	int m_fastGroups[NUMBER_OF_FAST_GROUPS];

	/** the fastest DefragmentationGroup object */
	int m_fastGroup;

	/** the number of active DefragmentationGroup objects */
	int m_numberOfActiveGroups;

	/** the number of fast DefragmentationGroup objects */
	int m_numberOfFastGroups;

	/** update m_fastGroup, if no DefragmentationGroup can allocate n elements, then m_fastGroup is set
	 to INVALID_GROUP */
	void getFastGroup(int n,int bytesPerElement,bool show);
public:
	/** allocate a SmallSmartPointer */
	SmallSmartPointer allocate(int n,int bytesPerElement,int*group);

	/** initialize the DefragmentationLane */
	void constructor(int number,int bytesPerElement,bool show);

	/** can the DefragmentationLane allocate n elements ? */
	bool canAllocate(int n,int bytesPerElement,bool show);

	/** return memory to the pool */
	void deallocate(SmallSmartPointer a);

	/** get the identifier of the DefragmentationLane */
	int getNumber();

	/** get a DefragmentationGroup directly */
	DefragmentationGroup*getGroup(int i);

};


#endif
