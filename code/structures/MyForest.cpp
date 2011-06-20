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

#include <structures/MyForest.h>
#include <cryptography/crypto.h>
#include <stdio.h>
#include <sstream>
using namespace std;

void MyForest::constructor(MyAllocator*allocator,int type,bool show){
	m_show=show;
	m_type=type;
	m_numberOfTrees=131072;
	m_allocator=allocator;
	#ifdef ASSERT
	assert(m_allocator!=NULL);
	#endif
	m_trees=(SplayTree<uint64_t,Vertex>*)__Malloc(sizeof(SplayTree<uint64_t,Vertex>)*m_numberOfTrees,m_type,m_show);
	for(int i=0;i<m_numberOfTrees;i++){
		m_trees[i].constructor();
	}
	m_inserted=false;
	m_size=0;
}

uint64_t MyForest::size(){
	return m_size;
}

int MyForest::getNumberOfTrees(){
	return m_numberOfTrees;
}

SplayTree<uint64_t,Vertex>*MyForest::getTree(int i){
	return m_trees+i;
}

int MyForest::getTreeIndex(uint64_t i){
	return uniform_hashing_function_2_64_64(i)%m_numberOfTrees;
}

Vertex*MyForest::find(uint64_t key){
	SplayNode<uint64_t,Vertex>*n=m_trees[getTreeIndex(key)].find(key,m_frozen);
	if(n!=NULL){
		return n->getValue();
	}
	return NULL;
}

Vertex*MyForest::insert(uint64_t key){
	int tree=getTreeIndex(key);
	#ifdef ASSERT
	assert(tree<m_numberOfTrees);
	#endif
	SplayNode<uint64_t,Vertex>*n=m_trees[tree].insert(key,m_allocator,&m_inserted);
	if(m_inserted){
		m_size++;
	}
	if(n!=NULL){
		return n->getValue();
	}
	return NULL;
}

bool MyForest::inserted(){
	return m_inserted;
}

void MyForest::freeze(){
	m_frozen=true;
}

void MyForest::unfreeze(){
	m_frozen=false;
}

void MyForest::show(int rank,const char*prefix){
	cout<<"MyForest::show()"<<endl;
	ostringstream a;
	a<<prefix<<".Forest.Rank_"<<rank<<".txt";
	FILE*f=fopen(a.str().c_str(),"w");
	for(int i=0;i<m_numberOfTrees;i++){
		fprintf(f,"%i %li\n",i,getTree(i)->size());
	}
	fclose(f);
}

bool MyForest::frozen(){
	return m_frozen;
}

void MyForest::remove(uint64_t a){
	#ifdef ASSERT
	assert(!frozen());
	#endif

	int tree=getTreeIndex(a);
	if(m_trees[tree].remove(a,true,m_allocator)){
		m_size--;
	}
}

MyAllocator*MyForest::getAllocator(){
	return m_allocator;
}
