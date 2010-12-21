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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#include<MyForestIterator.h>

void MyForestIterator::constructor(MyForest*a){
	m_forest=a;
	m_treeId=0;
	m_iterator.constructor(m_forest->getTree(m_treeId));
	while(!m_iterator.hasNext()){
		m_treeId++;
		if(m_treeId==m_forest->getNumberOfTrees()){
			break;
		}
		m_iterator.constructor(m_forest->getTree(m_treeId));
	}
}

bool MyForestIterator::hasNext() const{
	return m_iterator.hasNext();
}

SplayNode<uint64_t,Vertex>*MyForestIterator::next(){
	SplayNode<uint64_t,Vertex>*node=m_iterator.next();

	while(!m_iterator.hasNext()){
		m_treeId++;
		if(m_treeId==m_forest->getNumberOfTrees()){
			break;
		}
		m_iterator.constructor(m_forest->getTree(m_treeId));
	}
	return node;
}
