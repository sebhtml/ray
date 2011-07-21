/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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


#define ASSERT

#include<SplayTree.h>
#include<SplayTreeIterator.h>
#include<stdlib.h>
#include<assert.h>

void test_remove(){
	MyAllocator allocator;
	allocator.constructor(1000);
	SplayTree<int,int> a;
	a.constructor(&allocator);
	for(int k=0;k<100;k++){
		a.insert(k);
		assert(a.find(k)!=NULL);
	}
	assert(a.size()==100);

	a.remove(9);
	assert(a.size()==99);
	assert(a.find(9)==NULL);

	a.remove(50);
	assert(a.size()==98);
	assert(a.find(50)==NULL);

	a.remove(50);
	assert(a.size()==98);
	assert(a.find(50)==NULL);

	a.remove(-1);

	assert(a.size()==98);
}

void test_iterator(){
	MyAllocator allocator;
	allocator.constructor(1000);
	SplayTree<int,int> a;
	a.constructor(&allocator);
	SplayTreeIterator<int,int> b;
	b.constructor(&a);
	b.hasNext();
	b.hasNext();
	b.hasNext();
	b.next();
	a.insert(1);

	b.constructor(&a);
	b.hasNext();
	b.hasNext();
	b.hasNext();
	b.next();
	b.hasNext();
	b.hasNext();
}

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

	test_remove();
	test_iterator();
	return 0;
}
