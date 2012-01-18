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

#ifndef _DefragmentationGroup_H
#define _DefragmentationGroup_H

/* 
 * How many elements per group ?
 * this is the number of values of uint16_t */
#define ELEMENTS_PER_GROUP 65536 

/**
 * Number of fast pointers
 * Fast pointers is a list of SmallSmartPointer that can be used.
 */
#define FAST_POINTERS 256

#include <stdint.h>

/**
 * A SmallSmartPointer is a smart pointer than only a DefragmentationGroup
 * can resolve.
 */
typedef uint16_t SmallSmartPointer;

/**
 * This class does the complex logic
 * for memory allocation and deallocation,
 * including defragmentation using compacting
 * \author Sébastien Boisvert
 */
class DefragmentationGroup{
	/** freed stuff to accelerate things. */
	uint16_t m_fastPointers[FAST_POINTERS];
	
	/**
 * the number of available elements
 */
	int m_availableElements;

	/** last free position */
	int m_freeSliceStart;

	/**
 * 	Pointer to allocated memory
 * 	65536 * 18 = 1179648 bytes 
 */
	uint8_t*m_block;

	/**
 * 	the sizes range from 0 to 64
 * 	using a uint8_t for each SmallSmartPointer
 * 	65536 * 1 = 65536 bytes
 */
	uint8_t*m_allocatedSizes;

	/**
 * 	the offsets
 * 	these will change during defragmentation
 * 	128 KiB
 */
	uint16_t*m_allocatedOffsets;

/** print the bitmap
 */
	void print();

/**
 * get a SmallSmartPointer
 */
	SmallSmartPointer getAvailableSmallSmartPointer();
	
/*
 * returns true if defragmented something. */
	bool defragment(int bytesPerElement,uint16_t*cellContents,uint8_t*cellOccupancies);
public:


/**
 * Initialize pointers to NULL
 */
	void setPointers();

/** 
 * Initialiaze DefragmentationGroup
 */
	void constructor(int bytesPerElement,bool show);

/**
 * Allocate memory
 */
	SmallSmartPointer allocate(int n);

/**
 * Free memory
 * deallocate will defragment the block immediately
 */
	void deallocate(SmallSmartPointer a,int bytesPerElement,uint16_t*cellContents,uint8_t*cellOccupancies);
/** 
 * destroy the allocator
 */
	void destructor(bool show);

/**
 * can the allocator allocate n elements ?
 */
	bool canAllocate(int n);

	/**
 * 	translate a SmallSmartPointer to an actual pointer
 */
	void*getPointer(SmallSmartPointer a,int bytesPerElement);
	
	/**
 * 	return yes if activated
 */
	bool isOnline();

/**
 * return the number of available elements 
 */
	int getAvailableElements();

	int getFreeSliceStart();
};

#endif
