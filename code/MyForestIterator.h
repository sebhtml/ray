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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#ifndef _MyForestIterator
#define _MyForestIterator

#include<MyForest.h>
#include<SplayTreeIterator.h>
#include<SplayNode.h>
#include<Vertex.h>
#include<stdint.h>

class MyForestIterator{
	MyForest*m_forest;
	SplayTreeIterator<uint64_t,Vertex> m_iterator;
	int m_treeId;
public:
	void constructor(MyForest*a);
	bool hasNext() const;
	SplayNode<uint64_t,Vertex>*next();
};

#endif
