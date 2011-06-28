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
#include <memory/MyAllocator.h>
#include <iostream>
#include <assert.h>
using namespace std;

/**
 * This is an hash group.
 * MyHashTable contains many MyHashTableGroup.
 * Each MyHashTableGroup is responsible for a slice of buckets.
 */
template<class KEY,class VALUE>
class MyHashTableGroup{
	/**
 * 	The VALUEs
 */
	VALUE*m_vector;

	/**
 * 	the bitmap containing the presence of absence  of a bucket 
 */
	uint8_t*m_bitmap;

	/**
 * 	set a bit
 */
	void setBit(int i,int j);
public:
	/** requests the bucket directly
 * 	fails if it is empty
 * 	*/
	VALUE*getBucket(int bucket);

	/**
 * 	is the bucket utilised by someone else than key ?
 */
	bool bucketIsUtilisedBySomeoneElse(int bucket,KEY*key);

	/**
 * 	get the bit
 */
	int getBit(int i);

	/** 
 * 	build a group of buckets
 */
	void constructor(int numberOfBucketsInGroup,MyAllocator*allocator);

	/**
 * 	insert into the group
 * 	should not be called if the bucket is used by someone else already
 * 	otherwise it *will* fail
 */
	VALUE*insert(int numberOfBucketsInGroup,int bucket,KEY*key,MyAllocator*allocator,bool*inserted);

	/**
 * 	find a key
 * 	should not be called if the bucket is used by someone else already
 * 	otherwise it *will* fail
 */
	VALUE*find(int bucket,KEY*key);
};

/** sets the bit in the correct byte */
template<class KEY,class VALUE>
void MyHashTableGroup<KEY,VALUE>::setBit(int bit,int value){
	int byteId=bit/8;
	int bitInByte=bit%8;
	#ifdef ASSERT
	assert(value==1||value==0);
	#endif
	if(value==1)
		m_bitmap[byteId]|=(1<<bitInByte);
	else if(value==0)
		m_bitmap[byteId]&=(~(1<<bitInByte));
	#ifdef ASSERT
	assert(getBit(bit)==value);
	#endif
}

/** gets the bit  */
template<class KEY,class VALUE>
int MyHashTableGroup<KEY,VALUE>::getBit(int bit){
	int byteId=bit/8;
	int bitInByte=bit%8;
	uint64_t word=m_bitmap[byteId];
	word<<=(63-bitInByte);
	word>>=63;
	int bitValue=word;
	#ifdef ASSERT
	if(!(bitValue==0||bitValue==1))
		cout<<"Bit="<<bit<<" Bit value= "<<bitValue<<" originalByte="<<(int)m_bitmap[byteId]<<" ByteId="<<byteId<<" BitInByte="<<bitInByte<<endl;
	assert(bitValue==0||bitValue==1);
	#endif
	return bitValue;
}

/** builds the group of buckets */
template<class KEY,class VALUE>
void MyHashTableGroup<KEY,VALUE>::constructor(int numberOfBucketsInGroup,MyAllocator*allocator){
	m_vector=NULL;
	/* the number of buckets in a group must be a multiple of 8 */
	/* why ? => to save memory -- not to waste memory */
	#ifdef ASSERT
	assert(numberOfBucketsInGroup%8==0);
	#endif
	int requiredBytes=numberOfBucketsInGroup/8;
	m_bitmap=(uint8_t*)allocator->allocate(requiredBytes);
	
	/* set all bit to 0 */
	for(int i=0;i<numberOfBucketsInGroup;i++)
		setBit(i,0);
}

/**
 * inserts a key in a group
 * if the bucket is already utilised, then its key is key
 * if it is not used, key is added in bucket.
 */
template<class KEY,class VALUE>
VALUE*MyHashTableGroup<KEY,VALUE>::insert(int numberOfBucketsInGroup,int bucket,KEY*key,MyAllocator*allocator,bool*inserted){
	/* the bucket can not be used by another key than key */
	/* if it would be the case, then MyHashTable would not have sent key here */
	#ifdef ASSERT
	assert(!bucketIsUtilisedBySomeoneElse(bucket,key));
	#endif
	/*  count the number of occupied buckets before bucket */
	int bucketsBefore=0;
	for(int i=0;i<bucket;i++)
		bucketsBefore+=getBit(i);

	/* if the bucket is occupied, then it is returned immediately */
	if(getBit(bucket)==1){
		#ifdef ASSERT
		VALUE*value=m_vector+bucketsBefore;
		assert(value->m_lowerKey==*key);
		#endif
		return m_vector+bucketsBefore;
	}

	/* make sure that it is not occupied by some troll already */
	#ifdef ASSERT
	assert(getBit(bucket)==0);
	#endif
	/* the bucket is not occupied */
	setBit(bucket,1);

	/* compute the number of buckets to actually move. */
	int bucketsAfter=0;
	for(int i=bucket+1;i<numberOfBucketsInGroup;i++)
		bucketsAfter+=getBit(i);

	/* will allocate a new vector with one more element */
	int requiredBytes=(bucketsBefore+1+bucketsAfter)*sizeof(VALUE);
	VALUE*newVector=(VALUE*)allocator->allocate(requiredBytes);

	/* copy the buckets before */
	if(bucketsBefore>0)
		memcpy(newVector,m_vector,bucketsBefore*sizeof(VALUE));

	/* copy the buckets after */
	if(bucketsAfter>0)
		memcpy(newVector+bucketsBefore+1,m_vector+bucketsBefore,bucketsAfter*sizeof(VALUE));

	/* assign the new bucket */
	newVector[bucketsBefore].m_lowerKey=*key;
	
	/* garbage the old vector, MyAllocator will reuse it */
	if(m_vector!=NULL)
		allocator->free(m_vector,(bucketsBefore+bucketsAfter)*sizeof(VALUE));

	/* assign the vector */
	m_vector=newVector;

	*inserted=true;
	
	/* check that everything is OK now ! */
	#ifdef ASSERT
	assert(getBit(bucket)==1);
	if(getBucket(bucket)->m_lowerKey!=*key){
		cout<<"Expected"<<endl;
		key->print();
		cout<<"Actual"<<endl;
		getBucket(bucket)->m_lowerKey.print();
		
		cout<<"Bucket= "<<bucket<<" BucketsBefore= "<<bucketsBefore<<" BucketsAfter= "<<bucketsAfter<<endl;
	}
	assert(getBucket(bucket)->m_lowerKey==*key);
	assert(!bucketIsUtilisedBySomeoneElse(bucket,key));
	#endif
	return m_vector+bucketsBefore;
}

/** checks that the bucket is not utilised by someone else than key */
template<class KEY,class VALUE>
bool MyHashTableGroup<KEY,VALUE>::bucketIsUtilisedBySomeoneElse(int bucket,KEY*key){
	if(getBit(bucket)==0)
		return false;
	int bucketsBefore=0;
	for(int i=0;i<bucket;i++)
		bucketsBefore+=getBit(i);
	return m_vector[bucketsBefore].m_lowerKey!=*key;
}

/** get direct access to the bucket */
template<class KEY,class VALUE>
VALUE*MyHashTableGroup<KEY,VALUE>::getBucket(int bucket){
	/*  the bucket is not occupied therefore the key does not exist */
	#ifdef ASSERT
	assert(getBit(bucket)!=0);
	#endif
	/* compute the number of elements before bucket */
	int bucketsBefore=0;
	for(int i=0;i<bucket;i++)
		bucketsBefore+=getBit(i);

	/* return the bucket */
	return m_vector+bucketsBefore;
}

/** finds a key in a bucket  
 * this will *fail* if another key is found.
 * */
template<class KEY,class VALUE>
VALUE*MyHashTableGroup<KEY,VALUE>::find(int bucket,KEY*key){
	/*  the bucket is not occupied therefore the key does not exist */
	if(getBit(bucket)==0)
		return NULL;
	#ifdef ASSERT
	assert(!bucketIsUtilisedBySomeoneElse(bucket,key));
	#endif
	/* compute the number of elements before bucket */
	int bucketsBefore=0;
	for(int i=0;i<bucket;i++)
		bucketsBefore+=getBit(i);
	return m_vector+bucketsBefore;
}

/**
 * this hash table is specific to DNA
 * uses open addressing with quadratic probing 
 * class VALUE must have a public attribute of class KEY called m_lowerKey.
 * the number of seats can not be exceeded. 
 * based on the description at
 * \see http://google-sparsehash.googlecode.com/svn/trunk/doc/implementation.html
 */
template<class KEY,class VALUE>
class MyHashTable{
	/** 
 * 	chunk allocator
 */
	MyAllocator m_allocator;
	/** 
 * 	the  maximum probes required in quadratic probing
 */
	int m_maximumProbe;
	/**
 * Message-passing interface rank
 */
	int m_rank;
	/**
 * the number of seats in the theater */
	uint64_t m_totalNumberOfBuckets;
	/**
 * the number of people seated -- the number of utilised seats */
	uint64_t m_utilisedBuckets;
	/**
 * type of memory allocation */
	int m_mallocType;
	/**
 * memory allocation verbosity */
	bool m_showMalloc;
	/**
 * the actual seats */

	/**
 * 	groups of buckets
 */
	MyHashTableGroup<KEY,VALUE>*m_groups;

	/**
 * number of buckets per group
 */
	int m_numberOfBucketsInGroup;

	/**
 * 	number of groups 
 */
	int m_numberOfGroups;

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
	void constructor(uint64_t buckets,int mallocType,bool showMalloc,int rank);
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
VALUE*MyHashTable<KEY,VALUE>::at(uint64_t bucket){
	int group=bucket/m_numberOfBucketsInGroup;
	int bucketInGroup=bucket%m_numberOfBucketsInGroup;
	return m_groups[group].getBucket(bucketInGroup);
}

template<class KEY,class VALUE>
uint64_t MyHashTable<KEY,VALUE>::capacity(){
	return m_totalNumberOfBuckets;
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
	return m_utilisedBuckets;
}

/**
 * Allocate the theater and mark the seats as available
 */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::constructor(uint64_t buckets,int mallocType,bool showMalloc,int rank){
	m_rank=rank;
	
	int chunkSize=16777216;
	m_allocator.constructor(chunkSize,mallocType,showMalloc);
	
	m_mallocType=mallocType;
	m_showMalloc=showMalloc;
	/* the number of seats is a power of 2 */
	m_totalNumberOfBuckets=1;
	while(m_totalNumberOfBuckets<buckets)
		m_totalNumberOfBuckets*=2;
	m_utilisedBuckets=0;
	/* the number of buckets in a group */
	m_numberOfBucketsInGroup=48;

	m_numberOfGroups=(m_totalNumberOfBuckets-1)/m_numberOfBucketsInGroup+1;
	m_groups=(MyHashTableGroup<KEY,VALUE>*)m_allocator.allocate(m_numberOfGroups*sizeof(MyHashTableGroup<KEY,VALUE>));
	#ifdef ASSERT
	assert(m_groups!=NULL);
	#endif
	for(int i=0;i<m_numberOfGroups;i++)
		m_groups[i].constructor(m_numberOfBucketsInGroup,&m_allocator);
}

template<class KEY,class VALUE>
bool MyHashTable<KEY,VALUE>::isAvailable(uint64_t bucket){
	int group=bucket/m_numberOfBucketsInGroup;
	int bucketInGroup=bucket%m_numberOfBucketsInGroup;
	return m_groups[group].getBit(bucketInGroup)==0;
}

/**
 * finds a key
 */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::find(KEY*key){
	uint64_t probe=0; 
	uint64_t bucket=((hash_function_2(key)+quadraticProbe(probe))%m_totalNumberOfBuckets);
	int group=bucket/m_numberOfBucketsInGroup;
	int bucketInGroup=bucket%m_numberOfBucketsInGroup;
	while(m_groups[group].bucketIsUtilisedBySomeoneElse(bucketInGroup,key)){
		probe++;
		bucket=((hash_function_2(key)+quadraticProbe(probe))%m_totalNumberOfBuckets);
		group=bucket/m_numberOfBucketsInGroup;
		bucketInGroup=bucket%m_numberOfBucketsInGroup;
	}
	return m_groups[group].find(bucketInGroup,key);
}

/**
 * inserts a key
 */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::insert(KEY*key){
	uint64_t probe=0; 
	uint64_t bucket=((hash_function_2(key)+quadraticProbe(probe))%m_totalNumberOfBuckets);
	int group=bucket/m_numberOfBucketsInGroup;
	int bucketInGroup=bucket%m_numberOfBucketsInGroup;
	#ifdef ASSERT
	assert(group<m_numberOfGroups);
	#endif
	while(m_groups[group].bucketIsUtilisedBySomeoneElse(bucketInGroup,key)){
		probe++;
		bucket=((hash_function_2(key)+quadraticProbe(probe))%m_totalNumberOfBuckets);
		group=bucket/m_numberOfBucketsInGroup;
		bucketInGroup=bucket%m_numberOfBucketsInGroup;

		#ifdef ASSERT
		assert(group<m_numberOfGroups);
		#endif
	}
	if(probe>m_maximumProbe)
		m_maximumProbe=probe;
	bool inserted=false;
	m_groups[group].insert(m_numberOfBucketsInGroup,bucketInGroup,key,&m_allocator,&inserted);
	#ifdef ASSERT
	assert(m_groups[group].find(bucketInGroup,key)!=NULL);
	#endif

	if(inserted)
		m_utilisedBuckets++;

	/* check the load factor */
	check();

	VALUE*entry=find(key);
	#ifdef ASSERT
	assert(entry!=NULL);
	#endif
	return entry;
}

template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::destructor(){
	double loadFactor=(0.0+m_utilisedBuckets)/m_totalNumberOfBuckets;
	cout<<"Rank "<<m_rank<<": MyHashTable load factor: "<<loadFactor<<" maximum probe: "<<m_maximumProbe<<endl;
	m_allocator.clear();
	m_groups=NULL;
}

#endif
