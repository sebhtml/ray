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

/* the code in this file is based on my understanding (Sébastien Boisvert) of
 * the content of this page about Google sparse hash map.
 * \see http://google-sparsehash.googlecode.com/svn/trunk/doc/implementation.html
 *
 * 	double hashing is based on page 529 
 * 	of 
 * 	Donald E. Knuth
 * 	The Art of Computer Programming, Volume 3, Second Edition
 */

#ifndef _MyHashTable_H
#define _MyHashTable_H

/**
 * The precision of probe statistics.
 */
#define MAX_SAVED_PROBE 16

#include <stdint.h>
#include <time.h>
#include <memory/MyAllocator.h>
#include <iostream>
#include <assert.h>
using namespace std;

/**
 * This is an hash group.
 * MyHashTable contains many MyHashTableGroup.
 * Each MyHashTableGroup is responsible for a slice of buckets.
 * \see http://google-sparsehash.googlecode.com/svn/trunk/doc/implementation.html
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
	uint64_t m_bitmap;

	/**
 * 	set a bit
 */
	void setBit(int i,uint64_t j);
public:
	/** requests the bucket directly
 * 	fails if it is empty
 * 	*/
	VALUE*getBucket(int bucket);

/**
 * get the number of occupied buckets before bucket 
 */
	int getBucketsBefore(int bucket);

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
void MyHashTableGroup<KEY,VALUE>::setBit(int bit,uint64_t value){
/**
 * 	just set the bit to value in m_bitmap
 */
	#ifdef ASSERT
	assert(value==1||value==0);
	#endif
	
	/** use binary and or binary or */
	if(value==1){
		m_bitmap|=(value<<bit);
	}else if(value==0){
		value=1;
		m_bitmap&=(~(value<<bit));
	}

	/** make sure the bit is OK */
	#ifdef ASSERT
	if(getBit(bit)!=(int)value)
		cout<<"Bit="<<bit<<" Expected="<<value<<" Actual="<<getBit(bit)<<endl;
	assert(getBit(bit)==(int)value);
	#endif
}

/** gets the bit  */
template<class KEY,class VALUE>
int MyHashTableGroup<KEY,VALUE>::getBit(int bit){
	/* use a uint64_t word because otherwise bits are not correct */
	int bitValue=(m_bitmap<<(63-bit))>>63;

	/**  just a sanity check here */
	#ifdef ASSERT
	assert(bitValue==0||bitValue==1);
	#endif

	return bitValue;
}

/** builds the group of buckets */
template<class KEY,class VALUE>
void MyHashTableGroup<KEY,VALUE>::constructor(int numberOfBucketsInGroup,MyAllocator*allocator){
	/** we start with an empty m_vector */
	m_vector=NULL;

	/* the number of buckets in a group must be a multiple of 8 */
	/* why ? => to save memory -- not to waste memory */
	#ifdef ASSERT
	assert(numberOfBucketsInGroup%8==0);
	#endif

	/** set all bits to 0 */
	m_bitmap=0;
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
	int bucketsBefore=getBucketsBefore(bucket);

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

	#ifdef ASSERT
	int usedBucketsBefore=getBucketsBefore(64);
	#endif

	/* the bucket is not occupied */
	setBit(bucket,1);

	#ifdef ASSERT
	assert(getBucketsBefore(64)==usedBucketsBefore+1);
	#endif

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

	/** check that we inserted something somewhere actually */
	#ifdef ASSERT
	assert(find(bucket,key)!=NULL);
	assert(find(bucket,key)->m_lowerKey==*key);
	#endif

	/** tell the caller that we inserted something  somewhere */
	*inserted=true;
	return m_vector+bucketsBefore;
}

/** checks that the bucket is not utilised by someone else than key */
template<class KEY,class VALUE>
bool MyHashTableGroup<KEY,VALUE>::bucketIsUtilisedBySomeoneElse(int bucket,KEY*key){
	/** bucket is not used and therefore nobody uses it yet */
	if(getBit(bucket)==0)
		return false;

	/** get the number of buckets before */
	int bucketsBefore=getBucketsBefore(bucket);

	/** check if the key is the same */
	return m_vector[bucketsBefore].m_lowerKey!=*key;
}

/** get direct access to the bucket 
 * \pre the bucket must be used or the code fails
 * */
template<class KEY,class VALUE>
VALUE*MyHashTableGroup<KEY,VALUE>::getBucket(int bucket){
	/*  the bucket is not occupied therefore the key does not exist */
	#ifdef ASSERT
	assert(getBit(bucket)!=0);
	#endif

	/* compute the number of elements before bucket */
	int bucketsBefore=getBucketsBefore(bucket);

	/* return the bucket */
	return m_vector+bucketsBefore;
}

/** get the number of occupied buckets before bucket 
 * this is done by summing the bits of all buckets before bucket
 */
template<class KEY,class VALUE>
int MyHashTableGroup<KEY,VALUE>::getBucketsBefore(int bucket){
	/** the computation is done using only one variable and 
 * 	the one provided in argument */
	int bucketsBefore=0;
	bucket--;
	while(bucket>=0)
		bucketsBefore+=getBit(bucket--);
	return bucketsBefore;
}

/** finds a key in a bucket  
 * this will *fail* if another key is found.
 * */
template<class KEY,class VALUE>
VALUE*MyHashTableGroup<KEY,VALUE>::find(int bucket,KEY*key){
	/*  the bucket is not occupied therefore the key does not exist */
	if(getBit(bucket)==0){
		return NULL;
	}

	/** the bucket should contains key at this point */
	#ifdef ASSERT
	assert(!bucketIsUtilisedBySomeoneElse(bucket,key));
	#endif

	/* compute the number of elements before bucket */
	int bucketsBefore=getBucketsBefore(bucket);

	/** return the pointer m_vector with the offset valued by bucketsBefore */
	return m_vector+bucketsBefore;
}

/**
 * this hash table is specific to DNA
 * uses open addressing with double hashing
 * class VALUE must have a public attribute of class KEY called m_lowerKey.
 * the number of buckets can not be exceeded. 
 * based on the description at
 * \see http://google-sparsehash.googlecode.com/svn/trunk/doc/implementation.html
 */
template<class KEY,class VALUE>
class MyHashTable{
/**
 * the maximum acceptable load factor
 * beyond that value, the table is resized.
 */
	double m_maximumLoadFactor;

	/** 
 * 	chunk allocator
 */
	MyAllocator m_allocator;
	/** 
 * 	the  maximum probes required in probing
 */
	int m_probes[MAX_SAVED_PROBE];
	/**
 * Message-passing interface rank
 */
	int m_rank;
	/**
 * the number of buckets in the theater */
	uint64_t m_totalNumberOfBuckets;

	/**
 * the number of people seated -- the number of utilised buckets */
	uint64_t m_utilisedBuckets;
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
 * If the load factor is too high,
 * double the size, and regrow everything in the new one.
 */
	bool growIfNecessary();

/**
 * resize the hash table
 */
	void resize(uint64_t newSize);

/**
 * the type of memory allocation 
 */
	int m_mallocType;

/**
 * show the allocation events ?
 */
	bool m_showMalloc;

	/**
 * build the buckets and the hash */
	void constructor(uint64_t buckets,int mallocType,bool showMalloc,int rank);

public:
	/** unused constructor  */
	MyHashTable();
	/** unused constructor  */
	~MyHashTable();

	/**
 * 	is the seat available
 */
	bool isAvailable(uint64_t a);
	/**
 * build the buckets and the hash */
	void constructor(int mallocType,bool showMalloc,int rank);

	/**
 * find a seat given a key */
	VALUE*find(KEY*key);

	/**
 * insert a key */
	VALUE*insert(KEY*key);

/**
 * 	find a bucket given a key
 * 	This uses double hashing for open addressing.
 */
	void findBucketWithKey(KEY*key,uint64_t*probe,int*group,int*bucketInGroup);

	/**
 * burn the theater */
	void destructor();

	/**
 * 	get the number of occupied buckets 
 */
	uint64_t size();

	/**
 * 	get a bucket directly
 * 	will fail if empty
 */
	VALUE*at(uint64_t a);
	
	/** get the number of buckets */
	uint64_t capacity();

	/**
 * 	print the statistics of the hash table including,
 * 	but not necessary limited to:
 * 	- the load factor
 */
	void printStatistics();
};

/* get a bucket */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::at(uint64_t bucket){
	int group=bucket/m_numberOfBucketsInGroup;
	int bucketInGroup=bucket%m_numberOfBucketsInGroup;
	return m_groups[group].getBucket(bucketInGroup);
}

/** returns the number of buckets */
template<class KEY,class VALUE>
uint64_t MyHashTable<KEY,VALUE>::capacity(){
	return m_totalNumberOfBuckets;
}

/** makes the table grow if necessary */
template<class KEY,class VALUE>
bool MyHashTable<KEY,VALUE>::growIfNecessary(){
	/**  check if a growth is necessary */
	double loadFactor=(0.0+m_utilisedBuckets)/m_totalNumberOfBuckets;
	if(loadFactor<m_maximumLoadFactor)
		return false;

	/** double the size */
	uint64_t newSize=m_totalNumberOfBuckets*2;
	resize(newSize);

	/** indicates the caller that a sustainable growth occured */
	return true;
}

/** 
 * resize the whole hash table 
 */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::resize(uint64_t newSize){
	/** nothing to do because shrinking is not implemented */
	if(newSize<m_totalNumberOfBuckets)
		return;

	/** can not resize because newSize is too small */
	/** this case actually never happens */
	if(newSize<m_utilisedBuckets)
		return;

	/** get the starting time here */
	time_t startingTime=time(NULL);
	
	/** use a power of two */
	uint64_t power2=1;
	while(power2<newSize)
		power2*=2;
	newSize=power2;

	cout<<"Rank "<<m_rank<<" MyHashTable must grow now "<<m_totalNumberOfBuckets<<" -> "<<newSize<<endl;
	printStatistics();

	/** grow the thing. */
	MyHashTable<KEY,VALUE> newTable;

	#ifdef ASSERT
	assert(newSize>m_totalNumberOfBuckets);
	#endif

	/** build a new larger table */
	newTable.constructor(newSize,m_mallocType,m_showMalloc,m_rank);

	/** transfer all items in the new table */
	uint64_t bucket=0;
	while(bucket<capacity()){
		/** if the bucket is available, it means that it is empty */
		if(!isAvailable(bucket)){
			VALUE*entry=at(bucket);
			KEY*key=&(entry->m_lowerKey);

			/** save the size before for further use */
			#ifdef ASSERT
			uint64_t sizeBefore=newTable.size();
			#endif
			VALUE*insertedEntry=newTable.insert(key);

			/** assert that we inserted something somewhere */
			#ifdef ASSERT
			if(newTable.size()!=sizeBefore+1)
				cout<<"Expected: "<<sizeBefore+1<<" Actual: "<<newTable.size()<<endl;
			assert(newTable.size()==sizeBefore+1);
			#endif
		
			/** affect the value */
			*insertedEntry=*entry;
		}
		bucket++;
	}
	/** destroy the current table */
	destructor();

	/** copy important stuff  */
	m_groups=newTable.m_groups;
	m_totalNumberOfBuckets=newTable.m_totalNumberOfBuckets;
	m_allocator=newTable.m_allocator;
	m_numberOfGroups=newTable.m_numberOfGroups;

	/* m_utilisedBuckets  remains the same */
	#ifdef ASSERT
	if(m_utilisedBuckets!=newTable.m_utilisedBuckets)
		cout<<"Expected: "<<m_utilisedBuckets<<" Actual: "<<newTable.m_utilisedBuckets<<endl;
	assert(m_utilisedBuckets==newTable.m_utilisedBuckets);
	assert(m_allocator.getNumberOfChunks()==newTable.m_allocator.getNumberOfChunks());
	#endif

	/** copy probe profiles */
	for(int i=0;i<MAX_SAVED_PROBE;i++)
		m_probes[i]=newTable.m_probes[i];

	int seconds=time(NULL)-startingTime;

	/** indicates the caller that things changed places */
	cout<<"Rank "<<m_rank<<" MyHashTable growed well; elapsed time: "<<seconds<<" seconds"<<endl;
	printStatistics();
}

/** return the number of utilised buckets */
template<class KEY,class VALUE>
uint64_t MyHashTable<KEY,VALUE>::size(){
	return m_utilisedBuckets;
}

/**
 * Allocate the theater and mark the buckets as available
 */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::constructor(int mallocType,bool showMalloc,int rank){
	/** build the hash with a default size */
	uint64_t defaultSize=524288;
	constructor(defaultSize,mallocType,showMalloc,rank);
}

/** build the hash table given a number of buckets 
 * this is private actually 
 * */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::constructor(uint64_t buckets,int mallocType,bool showMalloc,int rank){
	/** this is the maximum acceptable load factor. */
	/** based on Figure 42 on page 531 of
 * 	The Art of Computer Programming, Second Edition, by Donald E. Knuth
 */
	m_maximumLoadFactor=0.7; /* 70.00% */

	/** set the message-passing interface rank number */
	m_rank=rank;
	
	/** give 16 MiB to the chunk allocator */
	int chunkSize=16777216;
	m_allocator.constructor(chunkSize,mallocType,showMalloc);
	m_mallocType=mallocType;
	m_showMalloc=showMalloc;

	/* use the provided number of buckets */
	/* the number of buckets is a power of 2 */
	m_totalNumberOfBuckets=buckets;

	m_utilisedBuckets=0;

	/* the number of buckets in a group
 * 	this is arbitrary I believe... 
 * 	in my case, 64 is the number of bits in a uint64_t
 * 	*/
	m_numberOfBucketsInGroup=64;

	/**
 * 	compute the required number of groups 
 */
	m_numberOfGroups=(m_totalNumberOfBuckets-1)/m_numberOfBucketsInGroup+1;

	/**
 * 	Compute the required number of bytes
 */
	int requiredBytes=m_numberOfGroups*sizeof(MyHashTableGroup<KEY,VALUE>);

	/** sanity check */
	#ifdef ASSERT
	if(requiredBytes<=0)
		cout<<"Groups="<<m_numberOfGroups<<" RequiredBytes="<<requiredBytes<<"  BucketsPower2: "<<m_totalNumberOfBuckets<<endl;
	assert(requiredBytes>=0);
	#endif

	/** allocate groups */
	m_groups=(MyHashTableGroup<KEY,VALUE>*)__Malloc(requiredBytes,
		mallocType,showMalloc);

	#ifdef ASSERT
	assert(m_groups!=NULL);
	#endif

	/** initialize groups */
	for(int i=0;i<m_numberOfGroups;i++)
		m_groups[i].constructor(m_numberOfBucketsInGroup,&m_allocator);

	/*  set probe profiles to 0 */
	for(int i=0;i<MAX_SAVED_PROBE;i++)
		m_probes[i]=0;
}

/** return yes if the bucket is empty, no otherwise */
template<class KEY,class VALUE>
bool MyHashTable<KEY,VALUE>::isAvailable(uint64_t bucket){
	int group=bucket/m_numberOfBucketsInGroup;
	int bucketInGroup=bucket%m_numberOfBucketsInGroup;
	
	#ifdef ASSERT
	assert(bucket<m_totalNumberOfBuckets);
	assert(group<m_numberOfGroups);
	assert(bucketInGroup<m_numberOfBucketsInGroup);
	#endif

	return m_groups[group].getBit(bucketInGroup)==0;
}

/** empty constructor, I use constructor instead  */
template<class KEY,class VALUE>
MyHashTable<KEY,VALUE>::MyHashTable(){}

/** empty destructor */
template<class KEY,class VALUE>
MyHashTable<KEY,VALUE>::~MyHashTable(){}

/**
 * find a bucket
 *
 * using probing, assuming a number of buckets that is a power of 2 
 * "But what is probing ?", you may ask.
 * This stuff is pretty simple actually.
 * Given an array of N elements and a key x and an hash
 * function HASH_FUNCTION, one simple way to get the bucket is
 *
 *  bucket =   HASH_FUNCTION(x)%N
 *
 * If a collision occurs (another key already uses bucket),
 * probing allows one to probe another bucket.
 *
 *  bucket =   ( HASH_FUNCTION(x) + probe(i) )%N
 *          
 *             where i is initially 0
 *             when there is a collision, increment i and check the new  bucket
 *             repeat until satisfied.
 */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::findBucketWithKey(KEY*key,uint64_t*probe,int*group,int*bucketInGroup){
	*probe=0; 
	/** 
 * 	double hashing is based on 
 * 	page 529 
 * 	of 
 * 	Donald E. Knuth
 * 	The Art of Computer Programming, Volume 3, Second Edition
 */
	/** between 0 and M-1 inclusively -- M is m_totalNumberOfBuckets */
	int64_t h1=hash_function_2(key)%m_totalNumberOfBuckets;
	uint64_t bucket=h1;
	(*group)=bucket/m_numberOfBucketsInGroup;

	/** double hashing is computed on probe 1, not on 0 */
	uint64_t h2=0;

	(*bucketInGroup)=bucket%m_numberOfBucketsInGroup;
	
	#ifdef ASSERT
	assert(bucket<m_totalNumberOfBuckets);
	assert(*group<m_numberOfGroups);
	assert(*bucketInGroup<m_numberOfBucketsInGroup);
	#endif

	/** probe bucket */
	while(m_groups[*group].bucketIsUtilisedBySomeoneElse(*bucketInGroup,key)){
		(*probe)++;
		
		/** issue a warning for an unexpected large probe depth */
		if((*probe)>256)
			cout<<"Rank "<<m_rank<<" Warning, probe depth is "<<*probe<<endl;

		/** only compute the double hashing once */
		if((*probe)==1){
 			/** between 1 and M-1 exclusive */
			/** maybe it works also when inclusive, but page 529 does not mention 
 * 			if it works inclusive for powers of 2 
 * 			hence, I assume it is exclusive for safety
 *
 * 			h(x)%M 		-> between 0 and M-1 inclusive
 * 			h(x)%(M-5) 	-> between 0 and M-4 inclusive
 * 			h(x)%(M-5)+2	-> between 2 and M-2 inclusive
 *
 * 			h(x)%(M-5)+"	-> between 1 and M-1 exclusive
 * 			*/
			h2=hash_function_1(key)%(m_totalNumberOfBuckets-5)+2;
			
			/** h2 can not be even */
			if(h2%2==0)
				h2--; 

			/* I assume it must be between 1 and M-1 exclusive */
			if(h2==1)
				h2=3;

			/** check boundaries */
			#ifdef ASSERT
			assert(h2!=0);
			assert(h2%2!=0);
			assert(h2>1);
			if(h2>=m_totalNumberOfBuckets-1)
				cout<<"h2= "<<h2<<" Buckets= "<<m_totalNumberOfBuckets<<endl;
			assert(h2<m_totalNumberOfBuckets-1);
			#endif
		}

		#ifdef ASSERT
		assert(h2!=0);
		#endif
		
		/** use double hashing */
		bucket=(h1+(*probe)*h2)%m_totalNumberOfBuckets;
		(*group)=bucket/m_numberOfBucketsInGroup;
		(*bucketInGroup)=bucket%m_numberOfBucketsInGroup;

		/** sanity checks */
		#ifdef ASSERT
		assert(bucket<m_totalNumberOfBuckets);
		assert(*group<m_numberOfGroups);
		assert(*bucketInGroup<m_numberOfBucketsInGroup);
		#endif
	}

	/** sanity checks */
	#ifdef ASSERT
	assert(bucket<m_totalNumberOfBuckets);
	assert(*group<m_numberOfGroups);
	assert(*bucketInGroup<m_numberOfBucketsInGroup);
	#endif
}

/**
 * finds a key
 */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::find(KEY*key){
	/** get the bucket */
	uint64_t probe;
	int group;
	int bucketInGroup;
	findBucketWithKey(key,&probe,&group,&bucketInGroup);

	#ifdef ASSERT
	assert(group<m_numberOfGroups);
	assert(bucketInGroup<m_numberOfBucketsInGroup);
	#endif
	
	/** ask the group to find the key */
	return m_groups[group].find(bucketInGroup,key);
}

/**
 * inserts a key
 */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::insert(KEY*key){
	#ifdef ASSERT
	uint64_t beforeSize=size();
	#endif

	/** find the group */
	uint64_t probe;
	int group;
	int bucketInGroup;
	findBucketWithKey(key,&probe,&group,&bucketInGroup);

	#ifdef ASSERT
	assert(group<m_numberOfGroups);
	assert(bucketInGroup<m_numberOfBucketsInGroup);
	#endif

	/* actually insert something somewhere */
	bool inserted=false;
	VALUE*entry=m_groups[group].insert(m_numberOfBucketsInGroup,bucketInGroup,key,&m_allocator,&inserted);

	/* check that nothing failed elsewhere */
	#ifdef ASSERT
	assert(entry!=NULL);
	assert(entry->m_lowerKey==*key);
	assert(m_groups[group].find(bucketInGroup,key)!=NULL);
	assert(m_groups[group].find(bucketInGroup,key)->m_lowerKey==*key);
	#endif

	/* increment the elements if an insertion occured */
	if(inserted){
		m_utilisedBuckets++;
		/* update the maximum number of probes */
		if(probe>(MAX_SAVED_PROBE-1))
			probe=(MAX_SAVED_PROBE-1);
		m_probes[probe]++;
	}

	#ifdef ASSERT
	assert(find(key)!=NULL);
	assert(find(key)->m_lowerKey==*key);
	if(inserted)
		assert(size()==beforeSize+1);
	#endif


	/* check the load factor */
	if(!growIfNecessary())
		return entry;

	/* must reprobe because everything changed */

	entry=find(key);
	#ifdef ASSERT
	assert(entry!=NULL);
	#endif
	return entry;
}

/** destroy the hash table */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::destructor(){
	m_allocator.clear();
	__Free(m_groups,m_mallocType,m_showMalloc);
}

/**
 * print handy statistics 
 */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::printStatistics(){
	double loadFactor=(0.0+m_utilisedBuckets)/m_totalNumberOfBuckets*100;
	cout<<"Rank "<<m_rank<<": MyHashTable, BucketGroups: "<<m_numberOfGroups<<", BucketsPerGroup: "<<m_numberOfBucketsInGroup<<", LoadFactor: "<<loadFactor<<"%, OccupiedBuckets: "<<m_utilisedBuckets<<"/"<<m_totalNumberOfBuckets<<endl;
	cout<<"Rank "<<m_rank<<" ProbeStatistics: ";
	for(int i=0;i<MAX_SAVED_PROBE;i++){
		if(m_probes[i]!=0)
			cout<<"("<<i<<"; "<<m_probes[i]<<"); ";
	}
	cout<<endl;
	m_allocator.print();
}

#endif
