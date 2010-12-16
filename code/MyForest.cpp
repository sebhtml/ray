/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include<MyForest.h>
#include<crypto.h>
#include<stdio.h>
#include<sstream>
using namespace std;

void MyForest::constructor(int count,MyAllocator*allocator){
	m_trees=(SplayTree<VERTEX_TYPE,Vertex>*)allocator->allocate(sizeof(SplayTree<VERTEX_TYPE,Vertex>)*count);
	for(int i=0;i<count;i++){
		m_trees[i].constructor();
		m_trees[i].constructor(allocator);
	}
	m_numberOfTrees=count;
	m_inserted=false;
	m_size=0;
}

int MyForest::size(){
	return m_size;
}

int MyForest::getNumberOfTrees(){
	return m_numberOfTrees;
}

SplayTree<VERTEX_TYPE,Vertex>*MyForest::getTree(int i){
	return m_trees+i;
}

int MyForest::getTreeIndex(VERTEX_TYPE i){
	return uniform_hashing_function_2_64_64(i)%m_numberOfTrees;
}

SplayNode<VERTEX_TYPE,Vertex>*MyForest::find(VERTEX_TYPE key){
	return m_trees[getTreeIndex(key)].find(key);
}

SplayNode<VERTEX_TYPE,Vertex>*MyForest::insert(VERTEX_TYPE key){
	int tree=getTreeIndex(key);
	SplayNode<VERTEX_TYPE,Vertex>*n=m_trees[tree].insert(key);
	m_inserted=m_trees[tree].inserted();
	if(m_inserted)
		m_size++;
	return n;
}

bool MyForest::inserted(){
	return m_inserted;
}

void MyForest::freeze(){
	for(int i=0;i<m_numberOfTrees;i++){
		m_trees[i].freeze();
	}
}

void MyForest::show(int rank,const char*prefix){
	cout<<"MyForest::show()"<<endl;
	ostringstream a;
	a<<prefix<<".Forest.Rank_"<<rank<<".txt";
	FILE*f=fopen(a.str().c_str(),"w+");
	for(int i=0;i<m_numberOfTrees;i++){
		fprintf(f,"%i %li\n",i,getTree(i)->size());
	}
	fclose(f);
}
