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

#ifndef _MyHashTable_H
#define _MyHashTable_H

#include <stdint.h>
#include <memory/allocator.h>
#include <assert.h>

#define _MyHashTable_SEAT_IS_AVAILABLE 0x7ea61822

/**
 * this hash table is specific to DNA
 * uses open addressing with quadratic probing 
 * class VALUE must have a public attribute of class KEY called m_lowerKey.
 * the number of seats can not be exceeded. 
 */
template<class KEY,class VALUE>
class MyHashTable{
	int m_maximumProbe;
	/**
 * Message-passing interface rank
 */
	int m_rank;
	/**
 * the number of seats in the theater */
	uint64_t m_totalNumberOfSeats;
	/**
 * the number of people seated -- the number of utilised seats */
	uint64_t m_utilisedSeats;
	/**
 * type of memory allocation */
	int m_mallocType;
	/**
 * memory allocation verbosity */
	bool m_showMalloc;
	/**
 * the actual seats */
	VALUE*m_seats;

	/**
 * quadratic probing, assuming a number of seats that is a power of 2 */
	uint64_t quadraticProbe(uint64_t i);
	void check();

public:
	/**
 * 	is the seat available
 */
	bool isAvailable(uint64_t a);
	/**
 * build the seats and the hash */
	void constructor(uint64_t seats,int mallocType,bool showMalloc,int rank);
	/**
 * find a seat given a key */
	VALUE*find(KEY*key);
	/**
 * insert a key */
	VALUE*insert(KEY*key);
	/**
 * burn the theater */
	void destructor();
	/**
 * 	get the number of occupied seats 
 */
	uint64_t size();

	VALUE*at(uint64_t a);
	uint64_t capacity();
};

template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::at(uint64_t a){
	return m_seats+a;
}

template<class KEY,class VALUE>
uint64_t MyHashTable<KEY,VALUE>::capacity(){
	return m_totalNumberOfSeats;
}

template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::check(){
}

/**
 * quadratic probing
 */
template<class KEY,class VALUE>
uint64_t MyHashTable<KEY,VALUE>::quadraticProbe(uint64_t i){
	return (i+i*i)/2; /* 0.5*i+0.5*i^2 */
}

template<class KEY,class VALUE>
uint64_t MyHashTable<KEY,VALUE>::size(){
	return m_utilisedSeats;
}

/**
 * Allocate the theater and mark the seats as available
 */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::constructor(uint64_t seats,int mallocType,bool showMalloc,int rank){
	m_rank=rank;

	/* needs at least this amount of bytes to ensure that the code for an available seat can be stored */
	assert(sizeof(KEY)>=8);
	m_mallocType=mallocType;
	m_showMalloc=showMalloc;
	/* the number of seats is a power of 2 */
	m_totalNumberOfSeats=1;
	while(m_totalNumberOfSeats<seats)
		m_totalNumberOfSeats*=2;
	m_utilisedSeats=0;
	uint64_t requiredBytes=m_totalNumberOfSeats*sizeof(VALUE);
	m_seats=(VALUE*)__Malloc(requiredBytes,m_mallocType,m_showMalloc);
	/* mark the seats as available */
	for(uint64_t i=0;i<m_totalNumberOfSeats;i++){
		VALUE*seat=m_seats+i;
		uint64_t*ptr=(uint64_t*)seat;
		*ptr=_MyHashTable_SEAT_IS_AVAILABLE;
	}
}

template<class KEY,class VALUE>
bool MyHashTable<KEY,VALUE>::isAvailable(uint64_t seat){
	return (*((uint64_t*)(m_seats+seat)))==_MyHashTable_SEAT_IS_AVAILABLE;
}

/**
 * finds a key
 */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::find(KEY*key){
	uint64_t probe=0; 
	uint64_t seat=((hash_function_2(key)+quadraticProbe(probe))%m_totalNumberOfSeats);
	while(probe<m_totalNumberOfSeats&& (*((uint64_t*)(m_seats+seat)))!=_MyHashTable_SEAT_IS_AVAILABLE){
		if(m_seats[seat].m_lowerKey==*key)
			return m_seats+seat;
		probe++;
		seat=((hash_function_2(key)+quadraticProbe(probe))%m_totalNumberOfSeats);
	}
	return NULL;
}

/**
 * inserts a key
 */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::insert(KEY*key){
	int probe=0;
	uint64_t seat=((hash_function_2(key)+quadraticProbe(probe))%m_totalNumberOfSeats);
	while((uint64_t)probe<m_totalNumberOfSeats&& (*((uint64_t*)(m_seats+seat)))!=_MyHashTable_SEAT_IS_AVAILABLE){
		if(m_seats[seat].m_lowerKey==*key)
			return m_seats+seat;
		probe++;
		seat=((hash_function_2(key)+quadraticProbe(probe))%m_totalNumberOfSeats);
	}
	m_seats[seat].m_lowerKey=*key;
	#ifdef ASSERT
	assert(m_utilisedSeats<m_totalNumberOfSeats);
	#endif
	m_utilisedSeats++;
	assert(!isAvailable(seat));
	if(probe>m_maximumProbe)
		m_maximumProbe=probe;
	check();

	return find(key);
}

template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::destructor(){
	double loadFactor=(0.0+m_utilisedSeats)/m_totalNumberOfSeats;
	cout<<"Rank "<<m_rank<<": MyHashTable, "<<m_totalNumberOfSeats<<" buckets, load factor: "<<loadFactor<<" maximum probe: "<<m_maximumProbe<<endl;
	__Free(m_seats,m_mallocType,m_showMalloc);
	m_seats=NULL;
	m_utilisedSeats=0;
	m_totalNumberOfSeats=0;
}

#endif
