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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include<MyForest.h>

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

SplayNode<VERTEX_TYPE,Vertex>*MyForest::find(VERTEX_TYPE key){
	return m_trees[hash6432(key)%m_numberOfTrees].find(key);
}

SplayNode<VERTEX_TYPE,Vertex>*MyForest::insert(VERTEX_TYPE key){
	int tree=hash6432(key)%m_numberOfTrees;
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
	for(int i=0;i<m_numberOfTrees;i++)
		m_trees[i].freeze();

	#ifdef SHOW_FOREST
	for(int i=0;i<m_numberOfTrees;i++)
		cout<<"Tree "<<i<<" has "<<m_trees[i].size()<<" items."<<endl;
	#endif
}
