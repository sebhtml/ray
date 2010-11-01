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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/



#ifndef _MyForest
#define _MyForest

#include<SplayTree.h>
#include<Vertex.h>
#include<MyAllocator.h>
#include<common_functions.h>

class MyForest{
	int m_numberOfTrees;
	u64 m_size;
	SplayTree<VERTEX_TYPE,Vertex>*m_trees;
	bool m_inserted;
public:
	void constructor(int count,MyAllocator*allocator);
	int size();
	int getNumberOfTrees();
	SplayTree<VERTEX_TYPE,Vertex>*getTree(int i);
	SplayNode<VERTEX_TYPE,Vertex>*find(VERTEX_TYPE key);
	SplayNode<VERTEX_TYPE,Vertex>*insert(VERTEX_TYPE key);
	bool inserted();
	void freeze();
};

#endif
