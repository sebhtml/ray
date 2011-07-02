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

#include <stdint.h>

/**
 * A SmallSmartPointer is a smart pointer than only a DefragmentationGroup
 * can resolve.
 */
typedef uint16_t SmallSmartPointer;

class DefragmentationGroup{
	/** number of available elements */
	uint32_t m_availableElements;
	/** last free position */
	uint16_t m_lastFreePosition;

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

	void defragment(uint16_t offset,uint8_t allocationLength);
public:
	void setPointers();
	void constructor(int period,bool show);
	SmallSmartPointer allocate(int n);
	void deallocate(SmallSmartPointer a);
	void destructor(bool show);
	bool canAllocate(int n);

	/**
 * 	translate a SmallSmartPointer to an actual pointer
 */
	void*getPointer(SmallSmartPointer a,int period);
	
	/**
 * 	return yes if activated
 */
	bool isOnline();

	int getAvailableElements();

	void print();
};

#endif
