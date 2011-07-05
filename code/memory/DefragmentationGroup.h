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

	/**
 *  	finally, the bitmap telling us what is utilised and what is not
 *	65536 bits = 1024 uint64_t = 8192 bytes
 */
	uint64_t*m_bitmap;
	
	/** 
 * 	get the bit for element a
 */
	int getBit(int a);

	/**
 * 	set the bit for element a
 */
	void setBit(int a,int b);

/**
 * 	Move all elements starting at newOffset+allocationLength up to m_freeSliceStart
 * 	by allocationLength positions on the left
 */
	void closeGap(int offset,int allocationLength,int bytesPerElement);

/**
 * find a gap 
 */
	void findGap(int*offset,int*length,int n);

/** print the bitmap
 */
	void print();

/**
 * get a SmallSmartPointer
 */
	SmallSmartPointer getAvailableSmallSmartPointer();
	
/**
 * find at least n AVAILABLE elements 
 */
	int findAtLeast(int n);

/**
 * Compact the block to create at least n consecutive elements 
 */
	void compact(int n,int bytesPerElement);

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
	SmallSmartPointer allocate(int n,int bytesPerElement);

/**
 * Free memory
 * deallocate will defragment the block immediately
 */
	void deallocate(SmallSmartPointer a,int bytesPerElement);
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
	
/*
 * returns true if defragmented something. */
	bool defragment(int bytesPerElement);

	int getAllocationSize(SmallSmartPointer a);
};

#endif
