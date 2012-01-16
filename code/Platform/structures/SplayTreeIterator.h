/*
 	Ray
    Copyright (C)  2010  Sébastien Boisvert

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

#ifndef _SplayTreeIterator
#define _SplayTreeIterator

#include<structures/MyStack.h>
#include<structures/SplayNode.h>
#include<structures/SplayTree.h>
#include<assert.h>
#include<stdlib.h>

/**
 * An iterator on SplayTree
 * \author Sébastien Boisvert
 */
template<class TREE_KEY_TYPE,class TREE_VALUE_TYPE>
class SplayTreeIterator{
	MyStack<SplayNode<TREE_KEY_TYPE,TREE_VALUE_TYPE>*>m_stack;
	SplayTree<TREE_KEY_TYPE,TREE_VALUE_TYPE>*m_tree;
	int m_processed;
	int m_id;
	int m_rank;
	int m_treeSize;
public:
	SplayTreeIterator(SplayTree<TREE_KEY_TYPE,TREE_VALUE_TYPE>*tree);
	void constructor(SplayTree<TREE_KEY_TYPE,TREE_VALUE_TYPE>*tree);
	SplayTreeIterator();
	bool hasNext() const;
	SplayNode<TREE_KEY_TYPE,TREE_VALUE_TYPE>*next();
	void setId(int a);
	void setRank(int i);
};


template<class TREE_KEY_TYPE,class TREE_VALUE_TYPE>
SplayTreeIterator<TREE_KEY_TYPE,TREE_VALUE_TYPE>::SplayTreeIterator(){
	m_tree=NULL;
	m_processed=-1;
	m_id=-1;
	m_treeSize=-1;
}

template<class TREE_KEY_TYPE,class TREE_VALUE_TYPE>
SplayTreeIterator<TREE_KEY_TYPE,TREE_VALUE_TYPE>::SplayTreeIterator(SplayTree<TREE_KEY_TYPE,TREE_VALUE_TYPE>*tree){
	constructor(tree);
}

template<class TREE_KEY_TYPE,class TREE_VALUE_TYPE>
void SplayTreeIterator<TREE_KEY_TYPE,TREE_VALUE_TYPE>::constructor(SplayTree<TREE_KEY_TYPE,TREE_VALUE_TYPE>*tree){
	#ifdef ASSERT
	assert(m_stack.size()==0);
	#endif
	m_tree=tree;
	m_processed=0;
	m_treeSize=m_tree->size();
	if(m_tree!=NULL && m_tree->getRoot()!=NULL){
		m_stack.push(tree->getRoot());
	}
}

template<class TREE_KEY_TYPE,class TREE_VALUE_TYPE>
bool SplayTreeIterator<TREE_KEY_TYPE,TREE_VALUE_TYPE>::hasNext()const{
	#ifdef ASSERT
	if(m_stack.size()==0 && m_tree!=NULL){
		if(m_processed!=(int)m_tree->size()){
			cout<<"Rank="<<m_rank<<" id="<<m_id<<" Processed="<<m_processed<<" Tree="<<m_tree->size()<<" onRecord="<<m_treeSize<<endl;
		}
		assert(m_processed==(int)m_tree->size());
	}
	#endif
	return m_stack.size()>0;
}

template<class TREE_KEY_TYPE,class TREE_VALUE_TYPE>
SplayNode<TREE_KEY_TYPE,TREE_VALUE_TYPE>*SplayTreeIterator<TREE_KEY_TYPE,TREE_VALUE_TYPE>::next(){
	if(hasNext()){
		SplayNode<TREE_KEY_TYPE,TREE_VALUE_TYPE>*c=m_stack.top();
		m_stack.pop();
		if(c->getLeft()!=NULL){
			m_stack.push(c->getLeft());
		}
		if(c->getRight()!=NULL){
			m_stack.push(c->getRight());
		}
		m_processed++;
		return c;
	}else{
		return NULL;
	}
}

template<class TREE_KEY_TYPE,class TREE_VALUE_TYPE>
void SplayTreeIterator<TREE_KEY_TYPE,TREE_VALUE_TYPE>::setId(int a){
	m_id=a;
}

template<class TREE_KEY_TYPE,class TREE_VALUE_TYPE>
void SplayTreeIterator<TREE_KEY_TYPE,TREE_VALUE_TYPE>::setRank(int rank){
	m_rank=rank;
}

#endif
