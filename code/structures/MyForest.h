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

#ifndef _MyForest
#define _MyForest

#include<structures/SplayTree.h>
#include<structures/Vertex.h>
#include<memory/MyAllocator.h>
#include<core/common_functions.h>
#include <memory/OnDiskAllocator.h>

/**
 * A forest contains numerous trees.
 */
class MyForest{
	int m_type;
	bool m_show;
	int m_numberOfTrees;
	uint64_t m_size;
	SplayTree<uint64_t,Vertex>*m_trees;
	bool m_inserted;
	int getTreeIndex(uint64_t i);
	MyAllocator*m_allocator;
	bool m_frozen;
public:
	int getNumberOfTrees();
	SplayTree<uint64_t,Vertex>*getTree(int i);
	void constructor(MyAllocator*allocator,int type,bool show);
	uint64_t size();
	Vertex*find(uint64_t key);
	Vertex*insert(uint64_t key);
	bool inserted();
	void freeze();
	void unfreeze();
	void show(int rank,const char*a);
	bool frozen();
	void remove(uint64_t a);
	MyAllocator*getAllocator();
};

#endif
