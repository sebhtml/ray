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

#ifndef _ChunkAllocatorWithDefragmentation_H
#define _ChunkAllocatorWithDefragmentation_H

#include <memory/DefragmentationGroup.h>
#include <memory/DefragmentationLane.h>
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
#define SmartPointer_NULL 4294967295u /* this is the maximum value for uint32_t */

#define NUMBER_OF_LANES 512

/**
 * This ChunkAllocatorWithDefragmentation  allocate memory with
 * a SmartPointer.
 * Inside, a SmartPointer is translated to a SmallSmartPointer and given to
 * the correct DefragmentationGroup in the  correct DefragmentationLane.
 * \author Sébastien Boisvert
 */
class ChunkAllocatorWithDefragmentation{
	DefragmentationLane*m_fastLane;
	int m_numberOfLanes;

	/** anxiliary table used by defragment() */
	uint16_t*m_cellContents;
	/** anxiliary table used by defragment() */
	uint8_t*m_cellOccupancies;

	/** show memory allocations */
	bool m_show;

	/** the size in bytes of an element   */
	int m_bytesPerElement;

	/** the first DefragmentationLane  */
	DefragmentationLane*m_defragmentationLanes[NUMBER_OF_LANES];

	/** update the fast lane */
	void updateFastLane(int n);
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

	ChunkAllocatorWithDefragmentation();
	~ChunkAllocatorWithDefragmentation();

	void defragment();
};

#endif
