#define ASSERT

#include<SplayTree.h>
#include<SplayTreeIterator.h>
#include<stdlib.h>
#include<assert.h>


int main(){
	MyAllocator allocator;
	allocator.constructor(1000);
	SplayTree<int,int> lol;
	lol.constructor(&allocator);
	srand(time(NULL));
	int n=10000000;
	int k=n;
	while(k--){
		lol.insert(k);
	}
	
	int i=0;
	SplayTreeIterator<int,int> iterator(&lol);
	while(iterator.hasNext()){
		SplayNode<int,int>*node=iterator.next();
		int v=node->getKey();
		i++;
	}

	lol.freeze();

	assert(i==n);
	assert(n==lol.size());
	SplayTreeIterator<int,int> iterator2(&lol);
	i=0;
	while(iterator2.hasNext()){
		SplayNode<int,int>*node=iterator2.next();
		int v=node->getKey();
		i++;
	}
	assert(i==n);
	assert(n==lol.size());

	return 0;
}
