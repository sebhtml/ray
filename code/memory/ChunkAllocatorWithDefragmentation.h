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

#ifndef _ChunkAllocatorWithDefragmentation_H
#define _ChunkAllocatorWithDefragmentation_H

#include <memory/DefragmentationGroup.h>
#include <stdlib.h>

/**
 * basically, a SmartPointer is just an handle
 * that can be resolved by some other object.
 * The underlying location can change, but the SmartPointer
 * does not change.
 *
 * A SmartPointer is transformed into a void* by ChunkAllocatorWithDefragmentation::getPointer()
 * getPointer actually just asks the appropriate DefragmentationGroup
 * by polling the array of DefragmentationLane objects.
 */
typedef uint32_t SmartPointer;

/**
 * This SmartPointer is the equivalent of NULL
 */
#define SmartPointer_NULL 4294967295 /* this is the maximum value for uint32_t */

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
typedef struct{
	/** list of DefragmentationGroup */
	DefragmentationGroup m_groups[GROUPS_PER_LANE];
	/** link to the next DefragmentationLane */
	void*m_next;
} DefragmentationLane;

/**
 * This ChunkAllocatorWithDefragmentation  allocate memory with
 * a SmartPointer.
 * Inside, a SmartPointer is translated to a SmallSmartPointer and given to
 * the correct DefragmentationGroup in the  correct DefragmentationLane.
 */
class ChunkAllocatorWithDefragmentation{
	/** the number of DefragmentationGroup that are active */
	int m_activeGroups;

	/** show memory allocations */
	bool m_show;

	/** the size in bytes of an element   */
	int m_period;

	/** the first DefragmentationLane  */
	DefragmentationLane*m_defragmentationLane;

	/** get the DefragmentationGroup of a SmartPointer */
	DefragmentationGroup*getGroup(SmartPointer a);
public:
	/**
 * print allocator information
 */
	void print();
	/** clear allocations */
	void destructor();

	/** initialize a ChunkAllocatorWithDefragmentation  */
	void constructor(int period,bool show);

/**
 * allocate memory
 */
	SmartPointer allocate(int s);

/**
 * free a SmartPointer
 */
	void deallocate(SmartPointer a);

/**
 * resolve a SmartPointer
 */
	void*getPointer(SmartPointer a);
};

#endif
