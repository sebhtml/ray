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

/* the code in this file is based on my understanding (Sébastien Boisvert) of
 * the content of this page about Google sparse hash map.
 * \see http://google-sparsehash.googlecode.com/svn/trunk/doc/implementation.html
 *
 * 	double hashing is based on page 529 
 * 	of 
 * 	Donald E. Knuth
 * 	The Art of Computer Programming, Volume 3, Second Edition
 *
 * Furthermore, it implements incremental resizing for real-time communication with low latency.
 */

#ifndef _MyHashTable_H
#define _MyHashTable_H

/**
 * The precision of probe statistics.
 */
#define MAX_SAVED_PROBE 16

#include <stdint.h>
#include <time.h>
#include <memory/ChunkAllocatorWithDefragmentation.h>
#include <iostream>
#include <assert.h>
using namespace std;

/**
 * This is an hash group.
 * MyHashTable contains many MyHashTableGroup.
 * Each MyHashTableGroup is responsible for a slice of buckets.
 * \see http://google-sparsehash.googlecode.com/svn/trunk/doc/implementation.html
 *
 * All operations are constant-time or so
 * \author Sébastien Boisvert
 */
template<class KEY,class VALUE>
class MyHashTableGroup{
	/**
 * 	The VALUEs
 */
	SmartPointer m_vector;

	/**
 * 	the bitmap containing the presence of absence  of a bucket 
 */
	uint64_t m_bitmap;

	/**
 * 	set a bit
 */
	void setBit(int i,uint64_t j);
public:
	void print(ChunkAllocatorWithDefragmentation*allocator);

	/** requests the bucket directly
 * 	fails if it is empty
 * 	*/
	VALUE*getBucket(int bucket,ChunkAllocatorWithDefragmentation*a);

/**
 * get the number of occupied buckets before bucket 
 */
	int getBucketsBefore(int bucket);

	/**
 * 	is the bucket utilised by someone else than key ?
 */
	bool bucketIsUtilisedBySomeoneElse(int bucket,KEY*key,ChunkAllocatorWithDefragmentation*allocator);

	/**
 * 	get the bit
 */
	int getBit(int i);

	/** 
 * 	build a group of buckets
 */
	void constructor(int numberOfBucketsInGroup,ChunkAllocatorWithDefragmentation*allocator);

	/**
 * 	insert into the group
 * 	should not be called if the bucket is used by someone else already
 * 	otherwise it *will* fail
 */
	VALUE*insert(int numberOfBucketsInGroup,int bucket,KEY*key,ChunkAllocatorWithDefragmentation*allocator,bool*inserted);

	/**
 * 	find a key
 * 	should not be called if the bucket is used by someone else already
 * 	otherwise it *will* fail
 */
	VALUE*find(int bucket,KEY*key,ChunkAllocatorWithDefragmentation*allocator);

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

	/* set bit to 1 */
	if(value==1){
		m_bitmap|=(value<<bit);

	/* set bit to 0 */
	}else if(value==0){
		uint64_t filter=1;
		filter<<=bit;
		filter=~filter;
		m_bitmap&=filter;
	}

	/** make sure the bit is OK */
	#ifdef ASSERT
	if(getBit(bit)!=(int)value)
		cout<<"Bit="<<bit<<" Expected="<<value<<" Actual="<<getBit(bit)<<endl;
	assert(getBit(bit)==(int)value);
	#endif
}

/* TODO replace  hard-coded 64 by m_numberOfBucketsInGroup  or numberOfBucketsInGroup */
template<class KEY,class VALUE>
void MyHashTableGroup<KEY,VALUE>::print(ChunkAllocatorWithDefragmentation*allocator){
	cout<<"Group bitmap"<<endl;
	for(int i=0;i<64;i++)
		cout<<" "<<i<<":"<<getBit(i);
	cout<<endl;
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
void MyHashTableGroup<KEY,VALUE>::constructor(int numberOfBucketsInGroup,ChunkAllocatorWithDefragmentation*allocator){
	/** we start with an empty m_vector */
	m_vector=SmartPointer_NULL;

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
VALUE*MyHashTableGroup<KEY,VALUE>::insert(int numberOfBucketsInGroup,int bucket,KEY*key,
	ChunkAllocatorWithDefragmentation*allocator,bool*inserted){

	/* the bucket can not be used by another key than key */
	/* if it would be the case, then MyHashTable would not have sent key here */
	#ifdef ASSERT
	if(bucketIsUtilisedBySomeoneElse(bucket,key,allocator)){
		cout<<"Error, bucket "<<bucket<<" is utilised by another key numberOfBucketsInGroup "<<numberOfBucketsInGroup<<" utilised buckets in group: "<<getBucketsBefore(numberOfBucketsInGroup)<<endl;
	}
	assert(!bucketIsUtilisedBySomeoneElse(bucket,key,allocator));
	#endif

	/*  count the number of occupied buckets before bucket */
	int bucketsBefore=getBucketsBefore(bucket);

	/* if the bucket is occupied, then it is returned immediately */
	if(getBit(bucket)==1){
		VALUE*vectorPointer=(VALUE*)allocator->getPointer(m_vector);
		#ifdef ASSERT
		VALUE*value=vectorPointer+bucketsBefore;
		assert(value->m_lowerKey==*key);
		#endif
		return vectorPointer+bucketsBefore;
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
	int requiredElements=(bucketsBefore+1+bucketsAfter);
	SmartPointer newVector=allocator->allocate(requiredElements);

	VALUE*newVectorPointer=(VALUE*)allocator->getPointer(newVector);
	VALUE*vectorPointer=(VALUE*)allocator->getPointer(m_vector);

	/* copy the buckets before */
	for(int i=0;i<bucketsBefore;i++)
		newVectorPointer[i]=vectorPointer[i];

	/* copy the buckets after */
	for(int i=0;i<bucketsAfter;i++)
		newVectorPointer[bucketsBefore+1+i]=vectorPointer[bucketsBefore+i];

	/* assign the new bucket */
	newVectorPointer[bucketsBefore].m_lowerKey=*key;
	
	/* garbage the old vector, ChunkAllocatorWithDefragmentation will reuse it */
	if(m_vector!=SmartPointer_NULL)
		allocator->deallocate(m_vector);

	/* assign the vector */
	m_vector=newVector;

	/* check that everything is OK now ! */
	#ifdef ASSERT
	assert(getBit(bucket)==1);
	if(getBucket(bucket,allocator)->m_lowerKey!=*key){
		cout<<"Expected"<<endl;
		key->print();
		cout<<"Actual"<<endl;
		getBucket(bucket,allocator)->m_lowerKey.print();
		
		cout<<"Bucket= "<<bucket<<" BucketsBefore= "<<bucketsBefore<<" BucketsAfter= "<<bucketsAfter<<endl;
	}
	assert(getBucket(bucket,allocator)->m_lowerKey==*key);
	assert(!bucketIsUtilisedBySomeoneElse(bucket,key,allocator));
	#endif

	/** check that we inserted something somewhere actually */
	#ifdef ASSERT
	assert(find(bucket,key,allocator)!=NULL);
	assert(find(bucket,key,allocator)->m_lowerKey==*key);
	#endif

	/** tell the caller that we inserted something  somewhere */
	*inserted=true;

	newVectorPointer=(VALUE*)allocator->getPointer(m_vector);
	return newVectorPointer+bucketsBefore;
}

/** checks that the bucket is not utilised by someone else than key */
template<class KEY,class VALUE>
bool MyHashTableGroup<KEY,VALUE>::bucketIsUtilisedBySomeoneElse(int bucket,KEY*key,
	ChunkAllocatorWithDefragmentation*allocator){
	/** bucket is not used and therefore nobody uses it yet */
	if(getBit(bucket)==0)
		return false;

	/** get the number of buckets before */
	int bucketsBefore=getBucketsBefore(bucket);

	/** check if the key is the same */
	VALUE*vectorPointer=(VALUE*)allocator->getPointer(m_vector);
	return vectorPointer[bucketsBefore].m_lowerKey!=*key;
}

/** get direct access to the bucket 
 * \pre the bucket must be used or the code fails
 * */
template<class KEY,class VALUE>
VALUE*MyHashTableGroup<KEY,VALUE>::getBucket(int bucket,ChunkAllocatorWithDefragmentation*allocator){
	/*  the bucket is not occupied therefore the key does not exist */
	#ifdef ASSERT
	assert(getBit(bucket)!=0);
	if(m_vector==SmartPointer_NULL)
		cout<<"bucket: "<<bucket<<endl;
	assert(m_vector!=SmartPointer_NULL);
	#endif

	/* compute the number of elements before bucket */
	int bucketsBefore=getBucketsBefore(bucket);

	/* return the bucket */
	VALUE*vectorPointer=(VALUE*)allocator->getPointer(m_vector);

	#ifdef ASSERT
	assert(vectorPointer!=NULL);
	#endif

	return vectorPointer+bucketsBefore;
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
VALUE*MyHashTableGroup<KEY,VALUE>::find(int bucket,KEY*key,ChunkAllocatorWithDefragmentation*allocator){

	/*  the bucket is not occupied therefore the key does not exist */
	if(getBit(bucket)==0){
		return NULL;
	}

	/** the bucket should contains key at this point */
	#ifdef ASSERT
	assert(!bucketIsUtilisedBySomeoneElse(bucket,key,allocator));
	#endif

	/* compute the number of elements before bucket */
	int bucketsBefore=getBucketsBefore(bucket);

	/** return the pointer m_vector with the offset valued by bucketsBefore */
	VALUE*vectorPointer=(VALUE*)allocator->getPointer(m_vector);
	return vectorPointer+bucketsBefore;
}

/**
 * this hash table is specific to DNA
 * uses open addressing with double hashing
 * class VALUE must have a public attribute of class KEY called m_lowerKey.
 * the number of buckets can not be exceeded. 
 * based on the description at
 * \see http://google-sparsehash.googlecode.com/svn/trunk/doc/implementation.html
 *
 * all operations are constant-time or so.
 *
 * probing depth is usually low because incremental resizing is triggered at load >= 70.0%
 * and double hashing eases exploration of bucket landscapes.
 */
template<class KEY,class VALUE>
class MyHashTable{
	/** is this table verbose */
	bool m_verbose;

	/** currently doing incremental resizing ? */
	bool m_resizing;

	/** auxiliary table for incremental resizing */
	MyHashTable*m_auxiliaryTableForIncrementalResize;

	/** current bucket to transfer */
	uint64_t m_currentBucketToTransfer;

/**
 * the maximum acceptable load factor
 * beyond that value, the table is resized.
 */
	double m_maximumLoadFactor;

	/** 
 * 	chunk allocator
 */
	ChunkAllocatorWithDefragmentation m_allocator;

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
         * Number of utilised buckets 
         */
	uint64_t m_utilisedBuckets;

/**
 * 	number of inserted elements
 * contains the sum of elements in the main table + the number of elements in the new
 * table but not in the old one
 * when not resizing, m_utilisedBuckets and m_size are the same
 * when resizing, m_size may be larger than m_utilisedBuckets
 */
	uint64_t m_size;

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
	void growIfNecessary();

/**
 * resize the hash table
 */
	void resize();

/**
 * the type of memory allocation 
 */
	char m_mallocType[100];

/**
 * show the allocation events ?
 */
	bool m_showMalloc;

	/**
 * build the buckets and the hash */
	void constructor(uint64_t buckets,const char*mallocType,bool showMalloc,int rank);

/*
 * find a key, but specify if the auxiliary table should be searched also */
	VALUE*findKey(KEY*key,bool checkAuxiliary);

public:
	
	void toggleVerbosity();

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
	void constructor(const char*mallocType,bool showMalloc,int rank);

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

	void printProbeStatistics();

/**
 * callback function for immediate defragmentation
 */
	void defragment();

	void completeResizing();

	bool needsToCompleteResizing();
};

/* get a bucket */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::at(uint64_t bucket){
	int group=bucket/m_numberOfBucketsInGroup;
	int bucketInGroup=bucket%m_numberOfBucketsInGroup;

	#ifdef ASSERT
	assert(m_groups[group].getBit(bucketInGroup)==1);
	#endif

	VALUE*e=m_groups[group].getBucket(bucketInGroup,&m_allocator);

	return e;
}

/** returns the number of buckets */
template<class KEY,class VALUE>
uint64_t MyHashTable<KEY,VALUE>::capacity(){
	return m_totalNumberOfBuckets;
}

/** makes the table grow if necessary */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::growIfNecessary(){
	if(m_resizing)
		return;

	/**  check if a growth is necessary */
	double loadFactor=(0.0+m_utilisedBuckets)/m_totalNumberOfBuckets;
	if(loadFactor<m_maximumLoadFactor)
		return;

	/** double the size */
	uint64_t newSize=m_totalNumberOfBuckets*2;

	/** nothing to do because shrinking is not implemented */
	if(newSize<m_totalNumberOfBuckets)
		return;

	/** can not resize because newSize is too small */
	/** this case actually never happens */
	if(newSize<m_utilisedBuckets)
		return;

	m_currentBucketToTransfer=0;
	m_resizing=true;

	#ifdef ASSERT
	assert(m_auxiliaryTableForIncrementalResize==NULL);
	#endif

	m_auxiliaryTableForIncrementalResize=new MyHashTable();

	/** build a new larger table */
	m_auxiliaryTableForIncrementalResize->constructor(newSize,m_mallocType,m_showMalloc,m_rank);

	//cout<<"Rank "<<m_rank<<" MyHashTable must grow now "<<m_totalNumberOfBuckets<<" -> "<<newSize<<endl;
	//printStatistics();

	#ifdef ASSERT
	assert(newSize>m_totalNumberOfBuckets);
	#endif
}

/** 
 * resize the whole hash table 
 */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::resize(){
	#ifdef ASSERT
	assert(m_resizing==true);
	assert(m_utilisedBuckets>0);
	#endif

 /*
 *	Suppose the main table has N slots.
 *	Then, the auxiliary has 2*N slots.
 *
 *	At each call to insert(), we call resize().
 *	Each call to resize() will transfert T items.
 *
 * 	In the worst case, the main table has exactly N items to transfert (all buckets are full).
 * 	Thus, this would require N/T calls to resize()
 *
 * 	resize() is actually called when insert() is called, so
 *	at completion of the transfer, the auxiliary table would have
 *
 *	N/T   (elements inserted directly in the auxiliary table)
 *	N     (elements copied from the main table to the auxiliary table)
 *
 *	Thus, when the transfert is completed, the auxiliary table has 
 *
 *	at most N+N/T elements.
 *
 * 	We have 
 *
 * 	N+N/T <= 2N		(equation 1.)
 * 	N+N/T -N  <= 2N -N
 * 	N/T <= N
 * 	N/T*1/N <= N * 1/N
 * 	1/T <= 1
 * 	1/T * T <= 1 * T
 * 	1 <= T
 *
 * 	So,  basically, as long as T >= 1, we are fine.
 *
 * 	But in our case, resize is triggered when the main table has at least
 * 	0.7*N
 *
 * 	So, we need 0.7*N/T calls to resizes.
 *
 * 	Upon completion, the auxiliary table will have
 *
 * 	0.7*N/T + 0.7*N
 *
 * 	The equation is then:
 *
 * 	0.7*N/T + 0.7*N <= 0.7*2*N 	equation 2.
 *
 * 	(we want the process to complete before auxiliary triggers its own incremental resizing !)
 *
 * 	(divide everything by 0.7)
 *
 * 	N/T+N<=2N
 *
 * 	(same equation as equation 1.)
 *
 * 	thus, T>=1
 *
 * 	Here, we choose T=2
 * 	*/

	int toProcess=2;

	int i=0;
	while(i<toProcess && m_currentBucketToTransfer<capacity()){
		/** if the bucket is available, it means that it is empty */
		if(!isAvailable(m_currentBucketToTransfer)){
			VALUE*entry=at(m_currentBucketToTransfer);

			#ifdef ASSERT
			assert(entry!=NULL);
			#endif

			KEY*key=&(entry->m_lowerKey);

			#ifdef ASSERT
			uint64_t probe;
			int group;
			int bucketInGroup;
			findBucketWithKey(key,&probe,&group,&bucketInGroup);
			uint64_t globalBucket=group*m_numberOfBucketsInGroup+bucketInGroup;
			
			if(globalBucket!=m_currentBucketToTransfer)
				cout<<"Expected: "<<m_currentBucketToTransfer<<" Actual: "<<globalBucket<<endl;
			assert(globalBucket==m_currentBucketToTransfer);

			if(find(key)==NULL)
				cout<<"Error: can not find the content of global bucket "<<m_currentBucketToTransfer<<endl;
			assert(find(key)!=NULL);
			#endif

			/** remove the key too */
			/* actually, can not remove anything because otherwise it will make the double hashing 
 * 			fails */

			/** save the size before for further use */
			#ifdef ASSERT
			uint64_t sizeBefore=m_auxiliaryTableForIncrementalResize->m_utilisedBuckets;

			/* the auxiliary should not contain the key already . */
			if(m_auxiliaryTableForIncrementalResize->find(key)!=NULL)
				cout<<"Moving globalBucket "<<m_currentBucketToTransfer<<" but the associated key is already in auxiliary table."<<endl;
			assert(m_auxiliaryTableForIncrementalResize->find(key)==NULL);
			#endif

			/* this pointer  will remain valid until the next insert. */
			VALUE*insertedEntry=m_auxiliaryTableForIncrementalResize->insert(key);
		
			/** affect the value */
			(*insertedEntry)=*entry;
	
			/** assert that we inserted something somewhere */
			#ifdef ASSERT
			if(m_auxiliaryTableForIncrementalResize->m_utilisedBuckets!=sizeBefore+1)
				cout<<"Expected: "<<sizeBefore+1<<" Actual: "<<m_auxiliaryTableForIncrementalResize->m_utilisedBuckets<<endl;
			assert(m_auxiliaryTableForIncrementalResize->m_utilisedBuckets==sizeBefore+1);
			#endif
		}
		i++;
		m_currentBucketToTransfer++;
	}

	/* we transfered everything */
	if(m_currentBucketToTransfer==capacity()){
		//cout<<"Rank "<<m_rank<<": MyHashTable incremental resizing is complete."<<endl;
		/* make sure the old table is now empty */
		#ifdef ASSERT
		//assert(size()==0);
		#endif

		/** destroy the current table */
		destructor();
	
		/** copy important stuff  */
		m_groups=m_auxiliaryTableForIncrementalResize->m_groups;
		m_totalNumberOfBuckets=m_auxiliaryTableForIncrementalResize->m_totalNumberOfBuckets;
		m_allocator=m_auxiliaryTableForIncrementalResize->m_allocator;
		m_numberOfGroups=m_auxiliaryTableForIncrementalResize->m_numberOfGroups;
		m_utilisedBuckets=m_auxiliaryTableForIncrementalResize->m_utilisedBuckets;
		m_size=m_auxiliaryTableForIncrementalResize->m_size;

		#ifdef ASSERT
		assert(m_auxiliaryTableForIncrementalResize->m_resizing == false);
		assert(m_size == m_utilisedBuckets);
		#endif

		/** copy probe profiles */
		for(int i=0;i<MAX_SAVED_PROBE;i++)
			m_probes[i]=m_auxiliaryTableForIncrementalResize->m_probes[i];
	
		/** indicates the caller that things changed places */

		//printStatistics();
		delete m_auxiliaryTableForIncrementalResize;
		m_auxiliaryTableForIncrementalResize=NULL;
		m_resizing=false;
	}
}

/** return the number of elements
 * contains the number of elements in the main table + those in the second that are not in the first */
template<class KEY,class VALUE>
uint64_t MyHashTable<KEY,VALUE>::size(){
	return m_size;
}

/**
 * Allocate the theater and mark the buckets as available
 */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::constructor(const char*mallocType,bool showMalloc,int rank){
	/** build the hash with a default size */
	uint64_t defaultSize=524288;
	constructor(defaultSize,mallocType,showMalloc,rank);
}

template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::toggleVerbosity(){
	m_verbose=!m_verbose;
}

/** build the hash table given a number of buckets 
 * this is private actually 
 * */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::constructor(uint64_t buckets,const char*mallocType,bool showMalloc,int rank){
	/** this is the maximum acceptable load factor. */
	/** based on Figure 42 on page 531 of
 * 	The Art of Computer Programming, Second Edition, by Donald E. Knuth
 */
	m_maximumLoadFactor=0.9; 

	m_auxiliaryTableForIncrementalResize=NULL;
	m_resizing=false;

	/** set the message-passing interface rank number */
	m_rank=rank;

	m_verbose=false;
	
	m_allocator.constructor(sizeof(VALUE),showMalloc);
	strcpy(m_mallocType,mallocType);

	m_showMalloc=showMalloc;

	/* use the provided number of buckets */
	/* the number of buckets is a power of 2 */
	m_totalNumberOfBuckets=buckets;

	m_size=0;

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
	/** 
 * 	double hashing is based on 
 * 	page 529 
 * 	of 
 * 	Donald E. Knuth
 * 	The Art of Computer Programming, Volume 3, Second Edition
 */
	(*probe)=0; 

	/** between 0 and M-1 inclusively -- M is m_totalNumberOfBuckets */
	uint64_t h1=key->hash_function_2()%m_totalNumberOfBuckets;

	/* first probe is 0 */
	/** double hashing is computed on probe 1, not on 0 */
	uint64_t h2=0;
	uint64_t bucket=h1;

	/** use double hashing */
	(*group)=bucket/m_numberOfBucketsInGroup;
	(*bucketInGroup)=bucket%m_numberOfBucketsInGroup;

	#ifdef ASSERT
	assert(bucket<m_totalNumberOfBuckets);
	assert(*group<m_numberOfGroups);
	assert(*bucketInGroup<m_numberOfBucketsInGroup);
	#endif

	/** probe bucket */
	while(m_groups[*group].bucketIsUtilisedBySomeoneElse(*bucketInGroup,key,&m_allocator)
		&& ((*probe)+1) < m_totalNumberOfBuckets){
		(*probe)++;
		
		/** the stride and the number of buckets are relatively prime because
			number of buckets = M = 2^m
		and the stride is 1 < s < M and is odd thus does not share factor with 2^m */

		#ifdef ASSERT
		/** issue a warning for an unexpected large probe depth */
		/** each bucket should be probed in exactly NumberOfBuckets probing events */
		if((*probe)>=m_totalNumberOfBuckets){
			cout<<"Rank "<<m_rank<<" Warning, probe depth is "<<(*probe)<<" h1="<<h1<<" h2="<<h2<<" m_totalNumberOfBuckets="<<m_totalNumberOfBuckets<<" m_size="<<m_size<<" m_utilisedBuckets="<<m_utilisedBuckets<<" bucket="<<bucket<<" m_resizing="<<m_resizing<<endl;
		}

		assert((*probe)<m_totalNumberOfBuckets);
		#endif

		/** only compute the double hashing once */
		if((*probe)==1){
			/**  page 529 
 * 				between 1 and M exclusive
				odd number
 * 			*/

			h2=key->hash_function_1()%m_totalNumberOfBuckets;
			
			/** h2 can not be 0 */
			if(h2 == 0)
				h2=3;

			/** h2 can not be even */
			if(h2%2 == 0)
				h2--; 

			/* I assume it must be between 1 and M exclusive */
			if(h2 < 3)
				h2=3;

			/** check boundaries */
			#ifdef ASSERT
			assert(h2!=0);
			assert(h2%2!=0);
			assert(h2%2 == 1);
			assert(h2>=3);
			if(h2>=m_totalNumberOfBuckets)
				cout<<"h2= "<<h2<<" Buckets= "<<m_totalNumberOfBuckets<<endl;
			assert(h2<m_totalNumberOfBuckets);
			assert((*probe)<m_totalNumberOfBuckets);
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
VALUE*MyHashTable<KEY,VALUE>::findKey(KEY*key,bool checkAuxiliary){ /* for verbosity */

	if(m_resizing && checkAuxiliary){
		/* check the new one first */
		VALUE*result=m_auxiliaryTableForIncrementalResize->find(key);
		if(result!=NULL)
			return result;
	}

	/** get the bucket */
	uint64_t probe;
	int group;
	int bucketInGroup;
	findBucketWithKey(key,&probe,&group,&bucketInGroup);

	#ifdef ASSERT
	assert(group<m_numberOfGroups);
	assert(bucketInGroup<m_numberOfBucketsInGroup);
	#endif
	
	if(m_verbose){
		cout<<"GridTable (service provided by MyHashTable) -> found key [";
		for(int i=0;i<key->getNumberOfU64();i++){
			if(i>0)
				cout<<" ";

			cout<<hex<<""<<key->getU64(i)<<dec;
		}
		cout<<"] on rank "<<m_rank;
		cout<<" in group "<<group<<" in bucket "<<bucketInGroup;
		cout<<" after "<<probe<<" probe operation";
		if(probe>1)
			cout<<"s";
		cout<<endl;
	}

	/** ask the group to find the key */
	return m_groups[group].find(bucketInGroup,key,&m_allocator);
}

/**
 * finds a key
 */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::find(KEY*key){
	#ifdef ASSERT
	assert(key!=NULL);
	#endif

	return findKey(key,true);
}

/**
 * inserts a key
 */
template<class KEY,class VALUE>
VALUE*MyHashTable<KEY,VALUE>::insert(KEY*key){
	/** do incremental resizing */
	if(m_resizing){
		resize();
	}
	if(m_resizing){
		/* check if key is not in the main table
 * 		insert it in the other one */
		if(findKey(key,false)==NULL){
			uint64_t sizeBefore=m_auxiliaryTableForIncrementalResize->m_utilisedBuckets;
			VALUE*e=m_auxiliaryTableForIncrementalResize->insert(key);
			if(m_auxiliaryTableForIncrementalResize->m_utilisedBuckets>sizeBefore){
				m_size++;
			}
			return e;
		}
	}

	#ifdef ASSERT
	uint64_t beforeSize=m_utilisedBuckets;
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
	if(entry->m_lowerKey!=*key)
		cout<<"Pointer: "<<entry<<endl;
	assert(entry->m_lowerKey==*key);
	assert(m_groups[group].find(bucketInGroup,key,&m_allocator)!=NULL);
	assert(m_groups[group].find(bucketInGroup,key,&m_allocator)->m_lowerKey==*key);
	#endif

	/* increment the elements if an insertion occured */
	if(inserted){
		m_utilisedBuckets++;
		m_size++;
		/* update the maximum number of probes */
		if(probe>(MAX_SAVED_PROBE-1))
			probe=(MAX_SAVED_PROBE-1);
		m_probes[probe]++;
	}

	#ifdef ASSERT
	assert(find(key)!=NULL);
	assert(find(key)->m_lowerKey==*key);
	if(inserted)
		assert(m_utilisedBuckets==beforeSize+1);
	#endif


	/* check the load factor */
	growIfNecessary();
	
	return entry;
}

/** destroy the hash table */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::destructor(){
	m_allocator.destructor();
	__Free(m_groups,m_mallocType,m_showMalloc);
}

/**
 * print handy statistics 
 */
template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::printStatistics(){
	//m_allocator.print();
}

template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::printProbeStatistics(){
	double loadFactor=(0.0+m_utilisedBuckets)/m_totalNumberOfBuckets*100;
	cout<<"Rank "<<m_rank<<": MyHashTable, BucketGroups: "<<m_numberOfGroups<<", BucketsPerGroup: "<<m_numberOfBucketsInGroup<<", LoadFactor: "<<loadFactor<<"%, OccupiedBuckets: "<<m_utilisedBuckets<<"/"<<m_totalNumberOfBuckets<<endl;
	cout<<"Rank "<<m_rank<<": incremental resizing in progress: ";
	if(m_resizing)
		cout<<"yes";
	else
		cout<<"no";
	cout<<endl;
	cout<<"Rank "<<m_rank<<" ProbeStatistics: ";
	for(int i=0;i<MAX_SAVED_PROBE;i++){
		if(m_probes[i]!=0)
			cout<<"("<<i<<"; "<<m_probes[i]<<"); ";
	}
}

template<class KEY,class VALUE>
void MyHashTable<KEY,VALUE>::completeResizing(){
	while(m_resizing)
		resize();

	#ifdef ASSERT
	assert(m_auxiliaryTableForIncrementalResize == NULL);
	#endif
}

template<class KEY,class VALUE>
bool MyHashTable<KEY,VALUE>::needsToCompleteResizing(){
	return m_resizing;
}

#endif
